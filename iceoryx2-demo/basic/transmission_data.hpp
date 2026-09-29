#pragma once

#include <cstdint>

// Shared-memory payloads must be self-contained. Do not place std::string,
// std::vector, owning pointers, or process-local addresses in this structure.
struct TransmissionData
{
    std::uint64_t sequence;
    std::int64_t value;
    double temperature;
};

