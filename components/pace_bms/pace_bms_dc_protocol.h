#pragma once

#include "pace_bms_protocol_base.h"

// T3/PBMS DC-control extension, kept separate from pack-specific analog/serial decoders.
// F5 requests a selector; its reply and F4 writes contain selector + big-endian uint16.
// Only the six explicitly requested current/power/limiting parameters are exposed.
class PaceBmsDcProtocol : public PaceBmsProtocolBase {
 public:
  static constexpr uint8_t BUS_CURRENT = 0x07;
  static constexpr uint8_t EQUALIZED_CHARGING_CURRENT = 0x0A;
  static constexpr uint8_t DISCHARGE_POWER = 0x0B;
  static constexpr uint8_t CHARGE_POWER = 0x0C;
  static constexpr uint8_t DISCHARGE_CURRENT_LIMITING = 0x02;
  static constexpr uint8_t CHARGE_CURRENT_LIMITING = 0x03;
  static constexpr uint8_t PARAMETERS[] = {BUS_CURRENT, EQUALIZED_CHARGING_CURRENT,
      DISCHARGE_POWER, CHARGE_POWER, DISCHARGE_CURRENT_LIMITING, CHARGE_CURRENT_LIMITING};

  PaceBmsDcProtocol(std::optional<uint8_t> version, std::optional<uint8_t> chemistry,
                    LogFuncPtr error, LogFuncPtr warning, LogFuncPtr info,
                    LogFuncPtr debug, LogFuncPtr verbose, LogFuncPtr very_verbose)
      : PaceBmsProtocolBase(0x25, std::nullopt, version, chemistry,
                            error, warning, info, debug, verbose, very_verbose) {}

  static bool IsSupportedParameter(uint8_t parameter) {
    for (uint8_t supported : PARAMETERS) if (parameter == supported) return true;
    return false;
  }

  static bool IsLimitingParameter(uint8_t parameter) {
    return parameter == DISCHARGE_CURRENT_LIMITING || parameter == CHARGE_CURRENT_LIMITING;
  }

  static float ValueScale(uint8_t parameter) {
    return (parameter == BUS_CURRENT || parameter == EQUALIZED_CHARGING_CURRENT) ? 10.0f : 1.0f;
  }

  static uint16_t MaxWriteRaw(uint8_t parameter) {
    if (parameter == BUS_CURRENT || parameter == EQUALIZED_CHARGING_CURRENT) return 1000;
    if (parameter == DISCHARGE_POWER || parameter == CHARGE_POWER) return 4800;
    return IsLimitingParameter(parameter) ? 1 : 0;
  }

  bool CreateReadRequest(uint8_t address, uint8_t parameter, std::vector<uint8_t>& request) {
    if (!IsSupportedParameter(parameter)) return false;
    std::vector<uint8_t> payload(2);
    uint16_t offset = 0;
    WriteHexEncodedByte(payload, offset, parameter);
    CreateRequest(address, 0xF5, payload, request);
    return true;
  }

  bool ProcessReadResponse(uint8_t address, std::optional<uint8_t> responding_address,
                           uint8_t parameter, const std::span<uint8_t>& response, uint16_t& raw) {
    if (!ValidateHexFrame(response)) return false;
    const int16_t length = ValidateResponseAndGetPayloadLength(address, responding_address, response);
    if (length < 0) return false;
    if (length != 6 || !IsSupportedParameter(parameter)) {
      LogError("DC F5 response must contain a parameter byte and a 16-bit value");
      return false;
    }
    uint16_t offset = PAYLOAD_START_OFFSET;
    if (ReadHexEncodedByte(response, offset) != parameter) {
      LogError("DC F5 response contains the wrong parameter ID");
      return false;
    }
    raw = ReadHexEncodedUShort(response, offset);
    if (IsLimitingParameter(parameter) && raw > 1) {
      LogError("DC limiting response must be 0000 (enabled) or 0001 (disabled)");
      return false;
    }
    return true;
  }

  bool CreateWriteEqualizedChargingCurrentRequest(uint8_t address, uint16_t deciamps,
                                                  std::vector<uint8_t>& request) {
    return CreateWriteRequest(address, EQUALIZED_CHARGING_CURRENT, deciamps, request);
  }

  bool CreateWriteRequest(uint8_t address, uint8_t parameter, uint16_t raw,
                          std::vector<uint8_t>& request) {
    if (!IsSupportedParameter(parameter) || raw > MaxWriteRaw(parameter)) return false;
    std::vector<uint8_t> payload(6);
    uint16_t offset = 0;
    WriteHexEncodedByte(payload, offset, parameter);
    WriteHexEncodedUShort(payload, offset, raw);
    CreateRequest(address, 0xF4, payload, request);
    return true;
  }

  bool ProcessWriteResponse(uint8_t address, std::optional<uint8_t> responding_address,
                            const std::span<uint8_t>& response) {
    // The captured success ACK has RTN=00 and an empty payload. Base validation
    // checks address, frame integrity, checksum, and the device return code.
    if (!ValidateHexFrame(response)) return false;
    const int16_t length = ValidateResponseAndGetPayloadLength(address, responding_address, response);
    if (length < 0) return false;
    if (length != 0) {
      LogError("DC F4 acknowledgement must have an empty payload");
      return false;
    }
    return true;
  }

 private:
  bool ValidateHexFrame(const std::span<uint8_t>& response) {
    if (response.size() < FRAME_SIZE_WITHOUT_PAYLOAD) {
      LogError("DC response is shorter than a complete frame");
      return false;
    }
    // Base nibble decoding logs non-hex characters but substitutes 0xF. Reject
    // them explicitly, including checksum digits, before using that decoder.
    for (size_t offset = 1; offset + 1 < response.size(); ++offset) {
      const uint8_t c = response[offset];
      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
        LogError("Invalid hex character in DC response");
        return false;
      }
    }
    return true;
  }
};
