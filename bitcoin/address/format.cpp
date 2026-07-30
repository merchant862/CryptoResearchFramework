#include "bitcoin/address/format.hpp"
#include "core/encoding/base58.hpp"
#include <cctype>
namespace crypto_research::bitcoin::address {
std::string_view to_string(PublicAddressType type) noexcept { switch(type) { case PublicAddressType::p2pkh:return "p2pkh"; case PublicAddressType::p2sh:return "p2sh"; case PublicAddressType::bech32:return "bech32"; default:return "unknown"; } }
PublicAddressFormat inspect_public_address(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
    PublicAddressFormat result{std::string{value}};
    if (value.rfind("bc1",0)==0 && value.size() >= 8U) { result.type=PublicAddressType::bech32; result.valid=true; return result; }
    if (value.empty() || (value.front()!='1' && value.front()!='3')) { result.error="unknown public address format"; return result; }
    try { (void)encoding::base58check_decode(value); result.type=value.front()=='1'?PublicAddressType::p2pkh:PublicAddressType::p2sh; result.valid=true; }
    catch(const std::exception& e) { result.error=e.what(); }
    return result;
}
}
