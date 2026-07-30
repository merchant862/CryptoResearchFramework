#include "bitcoin/address/format.hpp"
#include "core/encoding/base58.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace crypto_research::bitcoin::address {

namespace {

constexpr std::string_view kBech32Charset{"qpzry9x8gf2tvdw0s3jn54khce6mua7l"};

enum class Bech32Encoding { bech32, bech32m };

struct Bech32Validation final {
    bool valid{};
    std::string normalized;
    std::string error;
};

int charset_value(char c) {
    const auto pos = kBech32Charset.find(c);
    return pos == std::string_view::npos ? -1 : static_cast<int>(pos);
}

std::vector<std::uint8_t> hrp_expand(std::string_view hrp) {
    std::vector<std::uint8_t> values;
    values.reserve((hrp.size() * 2U) + 1U);
    for (const char c : hrp) {
        values.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(c) >> 5U));
    }
    values.push_back(0);
    for (const char c : hrp) {
        values.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(c) & 31U));
    }
    return values;
}

std::uint32_t polymod(const std::vector<std::uint8_t>& values) {
    static constexpr std::array<std::uint32_t, 5> generators{
        0x3b6a57b2U, 0x26508e6dU, 0x1ea119faU, 0x3d4233ddU, 0x2a1462b3U};

    std::uint32_t chk = 1;
    for (const auto value : values) {
        const auto top = chk >> 25U;
        chk = ((chk & 0x1ffffffU) << 5U) ^ value;
        for (std::size_t i = 0; i < generators.size(); ++i) {
            if (((top >> i) & 1U) != 0U) {
                chk ^= generators[i];
            }
        }
    }
    return chk;
}

std::optional<Bech32Encoding> verify_checksum(std::string_view hrp, const std::vector<std::uint8_t>& data) {
    auto values = hrp_expand(hrp);
    values.insert(values.end(), data.begin(), data.end());
    const auto checksum = polymod(values);
    if (checksum == 1U) {
        return Bech32Encoding::bech32;
    }
    if (checksum == 0x2bc830a3U) {
        return Bech32Encoding::bech32m;
    }
    return std::nullopt;
}

std::optional<std::vector<std::uint8_t>> convert_bits(const std::vector<std::uint8_t>& input) {
    std::vector<std::uint8_t> output;
    int accumulator = 0;
    int bits = 0;
    constexpr int from_bits = 5;
    constexpr int to_bits = 8;
    constexpr int max_value = (1 << to_bits) - 1;

    for (const auto value : input) {
        if ((value >> from_bits) != 0U) {
            return std::nullopt;
        }
        accumulator = (accumulator << from_bits) | value;
        bits += from_bits;
        while (bits >= to_bits) {
            bits -= to_bits;
            output.push_back(static_cast<std::uint8_t>((accumulator >> bits) & max_value));
        }
    }

    if (bits >= from_bits) {
        return std::nullopt;
    }
    if (((accumulator << (to_bits - bits)) & max_value) != 0) {
        return std::nullopt;
    }
    return output;
}

Bech32Validation validate_bech32_address(std::string_view value) {
    Bech32Validation result;
    result.normalized.reserve(value.size());

    if (value.size() < 8U || value.size() > 90U) {
        result.error = "invalid Bech32 length";
        return result;
    }

    bool has_lower = false;
    bool has_upper = false;
    for (const char c : value) {
        const auto ch = static_cast<unsigned char>(c);
        if (ch < 33U || ch > 126U) {
            result.error = "invalid Bech32 character range";
            return result;
        }
        has_lower = has_lower || std::islower(ch);
        has_upper = has_upper || std::isupper(ch);
        result.normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    if (has_lower && has_upper) {
        result.error = "mixed-case Bech32 address";
        return result;
    }

    const auto separator = result.normalized.rfind('1');
    if (separator == std::string::npos || separator == 0U || separator + 7U > result.normalized.size()) {
        result.error = "invalid Bech32 separator or checksum length";
        return result;
    }

    const auto hrp = std::string_view{result.normalized}.substr(0, separator);
    if (hrp != "bc") {
        result.error = "unsupported Bech32 human-readable part";
        return result;
    }

    std::vector<std::uint8_t> data;
    data.reserve(result.normalized.size() - separator - 1U);
    for (std::size_t i = separator + 1U; i < result.normalized.size(); ++i) {
        const auto value_index = charset_value(result.normalized[i]);
        if (value_index < 0) {
            result.error = "invalid Bech32 data character";
            return result;
        }
        data.push_back(static_cast<std::uint8_t>(value_index));
    }

    auto encoding = verify_checksum(hrp, data);
    if (!encoding) {
        result.error = "invalid Bech32 checksum";
        return result;
    }

    std::vector<std::uint8_t> payload{data.begin(), data.end() - 6};
    if (payload.empty()) {
        result.error = "missing witness version";
        return result;
    }

    const auto witness_version = payload.front();
    if (witness_version > 16U) {
        result.error = "invalid witness version";
        return result;
    }

    std::vector<std::uint8_t> program_values{payload.begin() + 1, payload.end()};
    auto witness_program = convert_bits(program_values);
    if (!witness_program) {
        result.error = "invalid witness program padding";
        return result;
    }
    if (witness_program->size() < 2U || witness_program->size() > 40U) {
        result.error = "invalid witness program length";
        return result;
    }

    if (witness_version == 0U) {
        if (*encoding != Bech32Encoding::bech32) {
            result.error = "version 0 witness address must use Bech32 checksum";
            return result;
        }
        if (witness_program->size() != 20U && witness_program->size() != 32U) {
            result.error = "version 0 witness program must be 20 or 32 bytes";
            return result;
        }
    } else if (*encoding != Bech32Encoding::bech32m) {
        result.error = "version 1+ witness address must use Bech32m checksum";
        return result;
    }

    result.valid = true;
    return result;
}

bool has_bc1_prefix(std::string_view value) {
    return value.size() >= 3U &&
           static_cast<char>(std::tolower(static_cast<unsigned char>(value[0]))) == 'b' &&
           static_cast<char>(std::tolower(static_cast<unsigned char>(value[1]))) == 'c' &&
           value[2] == '1';
}

} // namespace

std::string_view to_string(PublicAddressType type) noexcept { switch(type) { case PublicAddressType::p2pkh:return "p2pkh"; case PublicAddressType::p2sh:return "p2sh"; case PublicAddressType::bech32:return "bech32"; default:return "unknown"; } }

PublicAddressFormat inspect_public_address(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
    PublicAddressFormat result;
    result.normalized = std::string{value};
    if (has_bc1_prefix(value)) {
        auto bech32 = validate_bech32_address(value);
        result.normalized = std::move(bech32.normalized);
        result.type = PublicAddressType::bech32;
        result.valid = bech32.valid;
        result.error = std::move(bech32.error);
        return result;
    }
    if (value.empty() || (value.front()!='1' && value.front()!='3')) { result.error="unknown public address format"; return result; }
    try { (void)encoding::base58check_decode(value); result.type=value.front()=='1'?PublicAddressType::p2pkh:PublicAddressType::p2sh; result.valid=true; }
    catch(const std::exception& e) { result.error=e.what(); }
    return result;
}
}
