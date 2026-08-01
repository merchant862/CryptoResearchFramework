#include "bitcoin/address/segwit.hpp"

#include "core/encoding/base58.hpp"
#include "core/hashing/ripemd160.hpp"
#include "core/hashing/sha256.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace crypto_research::bitcoin::address {
    namespace {

        // ------------------------------------------------------------------
        // Bech32 / Bech32m encoding (BIP141 / BIP350)
        // ------------------------------------------------------------------
        constexpr std::string_view BECH32_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

        constexpr std::array<std::uint32_t, 5> BECH32_GENERATOR = {
            0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3
        };

        std::uint32_t bech32_polymod(const std::vector<std::uint8_t>& values) {
            std::uint32_t chk = 1;
            for (const auto v : values) {
                std::uint32_t b = chk >> 25;
                chk = ((chk & 0x1ffffff) << 5) ^ v;
                for (std::size_t i = 0; i < 5; ++i) {
                    if ((b >> i) & 1) chk ^= BECH32_GENERATOR[i];
                }
            }
            return chk;
        }

        std::vector<std::uint8_t> bech32_hrp_expand(std::string_view hrp) {
            std::vector<std::uint8_t> result;
            result.reserve(hrp.size() * 2 + 1);
            for (const auto c : hrp) {
                result.push_back(static_cast<std::uint8_t>(c) >> 5);
            }
            result.push_back(0);
            for (const auto c : hrp) {
                result.push_back(static_cast<std::uint8_t>(c) & 0x1f);
            }
            return result;
        }

        std::vector<std::uint8_t> convert_bits(std::span<const std::byte> data,
            std::size_t from_bits,
            std::size_t to_bits,
            bool pad) {
            std::vector<std::uint8_t> result;
            std::size_t acc = 0;
            std::size_t bits = 0;
            constexpr std::size_t maxv = (1 << 5) - 1;
            for (const auto value : data) {
                acc = (acc << from_bits) | std::to_integer<std::uint8_t>(value);
                bits += from_bits;
                while (bits >= to_bits) {
                    bits -= to_bits;
                    result.push_back(static_cast<std::uint8_t>((acc >> bits) & maxv));
                }
            }
            if (pad) {
                if (bits > 0) {
                    result.push_back(static_cast<std::uint8_t>((acc << (to_bits - bits)) & maxv));
                }
            }
            else if (bits >= from_bits || ((acc << (to_bits - bits)) & maxv)) {
                throw std::invalid_argument("invalid padding in convert_bits");
            }
            return result;
        }

        std::string bech32_encode(std::string_view hrp, std::span<const std::uint8_t> data, std::uint32_t checksum_constant) {
            auto values = bech32_hrp_expand(hrp);
            values.insert(values.end(), data.begin(), data.end());
            const auto n = values.size();
            values.resize(n + 6, 0);
            const auto polymod = bech32_polymod(values) ^ checksum_constant;
            for (std::size_t i = 0; i < 6; ++i) {
                values[n + i] = (polymod >> (5 * (5 - i))) & 0x1f;
            }
            std::string result(hrp);
            result.push_back('1');
            for (const auto v : values) {
                result.push_back(BECH32_CHARSET[v]);
            }
            return result;
        }

        // ------------------------------------------------------------------
        // Hash160 helper
        // ------------------------------------------------------------------
        std::vector<std::byte> hash160(std::span<const std::byte> data) {
            const auto sha = hashing::sha256(data);
            const auto ripemd = hashing::ripemd160(sha);
            return { ripemd.begin(), ripemd.end() };
        }

    } // namespace

    std::string p2sh_address(std::span<const std::byte> public_key) {
        if (public_key.size() != 33U) throw std::invalid_argument("P2SH requires 33-byte compressed public key");

        const auto pubkey_hash = hash160(public_key);

        // Redeem script: 0x00 0x14 <20-byte-hash>
        std::vector<std::byte> redeem_script;
        redeem_script.reserve(22U);
        redeem_script.push_back(std::byte{ 0x00 });
        redeem_script.push_back(std::byte{ 0x14 });
        redeem_script.insert(redeem_script.end(), pubkey_hash.begin(), pubkey_hash.end());

        const auto script_hash = hash160(redeem_script);

        // Version byte 0x05 for mainnet P2SH
        std::vector<std::byte> payload;
        payload.reserve(21U);
        payload.push_back(std::byte{ 0x05 });
        payload.insert(payload.end(), script_hash.begin(), script_hash.end());

        return encoding::base58check_encode(payload);
    }

    std::string bech32_address(std::span<const std::byte> public_key) {
        if (public_key.size() != 33U) throw std::invalid_argument("Bech32 requires 33-byte compressed public key");

        const auto pubkey_hash = hash160(public_key);

        // Witness program version 0, followed by 20-byte hash
        std::vector<std::uint8_t> witness_program;
        witness_program.push_back(0); // version 0
        const auto converted = convert_bits(pubkey_hash, 8, 5, true);
        witness_program.insert(witness_program.end(), converted.begin(), converted.end());

        // BIP141 Bech32 checksum constant = 1
        return bech32_encode("bc", witness_program, 1);
    }

    std::string bech32m_address(std::span<const std::byte> public_key) {
        if (public_key.size() != 33U) throw std::invalid_argument("Bech32m requires 33-byte compressed public key");

        const auto pubkey_hash = hash160(public_key);

        std::vector<std::uint8_t> witness_program;
        witness_program.push_back(0);
        const auto converted = convert_bits(pubkey_hash, 8, 5, true);
        witness_program.insert(witness_program.end(), converted.begin(), converted.end());

        // BIP350 Bech32m checksum constant = 0x2bc830a3
        return bech32_encode("bc", witness_program, 0x2bc830a3);
    }

} // namespace crypto_research::bitcoin::address