// Implementation of CapabilityDetector
#include "plugins/plugin_interface.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace desktop_tui {

uint32_t CapabilityDetector::detect_capabilities(const std::string& capabilities) {
    uint32_t result = 0;

    if (capabilities.empty()) {
        return result;
    }

    std::istringstream iss(capabilities);
    std::string token;

    while (iss >> token) {
        // Remove quotes if present
        if (token.front() == '"' && token.back() == '"') {
            token = token.substr(1, token.size() - 2);
        }

        // Try to match known capabilities
        for (int i = 0; i < 7; i++) {  // 0-6 are the defined capabilities
            if (CapabilityDetector::capability_name(static_cast<Capability>(i)) == token) {
                result |= static_cast<uint32_t>(static_cast<Capability>(i));
                break;
            }
        }
    }

    return result;
}

bool CapabilityDetector::is_valid_capability(Capability capability) {
    return capability >= Capability::None && capability <= Capability::All;
}

std::string CapabilityDetector::capability_name(Capability capability) {
    switch (capability) {
        case Capability::None:
            return "None";
        case Capability::ReadConfig:
            return "ReadConfig";
        case Capability::WriteConfig:
            return "WriteConfig";
        case Capability::SpawnProcess:
            return "SpawnProcess";
        case Capability::AccessClipboard:
            return "AccessClipboard";
        case Capability::AccessNetwork:
            return "AccessNetwork";
        case Capability::AccessFiles:
            return "AccessFiles";
        case Capability::AccessHardware:
            return "AccessHardware";
        case Capability::All:
            return "All";
        default:
            return "Unknown";
    }
}

std::string CapabilityDetector::describe_capabilities(uint32_t capabilities) {
    std::ostringstream oss;

    if (capabilities == static_cast<uint32_t>(Capability::All)) {
        oss << "All capabilities";
        return oss.str();
    }

    if (capabilities == 0) {
        oss << "No capabilities";
        return oss.str();
    }

    bool first = true;
    for (int i = 0; i < 7; i++) {
        if (capabilities & static_cast<uint32_t>(static_cast<Capability>(i))) {
            if (!first) {
                oss << ", ";
            }
            oss << CapabilityDetector::capability_name(static_cast<Capability>(i));
            first = false;
        }
    }

    return oss.str();
}

} // namespace desktop_tui
