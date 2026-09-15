#pragma once 
#include "toml.hpp"
#include "navigation.hpp"
#include <filesystem>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>


struct Metadata {
    // Image dimensions
    unsigned int num_lines;
    unsigned int num_columns;

    // Navigation offsets
    int line_offset;
    int column_offset;

    // Actual product resolution
    MAGIC_EXACT resolution;

    // Satellite-specific navigation
    MAGIC_EXACT satellite_radius_km;
    MAGIC_EXACT scan_scale_numerator;
    MAGIC_EXACT scan_scale_denominator;

    // Satellite-specific timing
    MAGIC_EXACT full_disk_scan_min;

    // Spectral information
    int wavelength;

    // Configuration tables
    std::unordered_map<int, MAGIC_EXACT> resolution_factors;
    std::unordered_map<std::string, int> wavelengths;
};

inline Metadata loadMetadata(const std::string& channel){
    const char* satellite_env = std::getenv("SATELLITE");

    if (satellite_env == nullptr || std::string(satellite_env).empty()) {
        throw std::runtime_error(
            "SATELLITE environment variable is not set");
    }

    std::string satellite = satellite_env;

    std::filesystem::path config_path =
        std::filesystem::path("satellites") /
        (satellite + ".toml");

    if (!std::filesystem::exists(config_path)) {
        throw std::runtime_error(
            "Satellite configuration file not found: " +
            config_path.string());
    }

    toml::table config = toml::parse_file(config_path.string());

    Metadata metadata;

    metadata.satellite_radius_km = config["navigation"]["satellite_radius_km"].value_or(0.0);
    metadata.scan_scale_numerator = config["navigation"]["scan_scale_numerator"].value_or(0.0);
    metadata.scan_scale_denominator = config["navigation"]["scan_scale_denominator"].value_or(0.0);
    metadata.full_disk_scan_min = config["navigation"]["full_disk_scan_min"].value_or(0.0);
    // ------------------------------------------------------------
    // Resolution factors
    // ------------------------------------------------------------

    if (const auto* factors = config["navigation"]["resolution_factors"].as_table()) {
        for (const auto& [key, value] : *factors) {
            const int resolution = std::stoi(std::string(key));

            if (const auto factor = value.value<MAGIC_EXACT>()) {
                metadata.resolution_factors[resolution] = *factor;
            }
        }
    }
    // ------------------------------------------------------------
    // Channel wavelengths
    // ------------------------------------------------------------

    if (auto* channels = config["channels"].as_table()) {
        for (const auto& [key, value] : *channels) {
            if (const auto wavelength = value.value<int>()) {
                metadata.wavelengths[
                    std::string(key)
                ] = *wavelength;
            }
        }
    }

    // ------------------------------------------------------------
    // Current channel
    // ------------------------------------------------------------

    auto wavelength_it = metadata.wavelengths.find(channel);

    if (wavelength_it == metadata.wavelengths.end()) {
        throw std::runtime_error("Channel '" + channel + "' is not defined in " + config_path.string());
    }

    metadata.wavelength = wavelength_it->second;

    return metadata;
}