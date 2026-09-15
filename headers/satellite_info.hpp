#pragma once 
#include "toml.hpp"
#include "navigation.hpp"
#include <filesystem>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>


struct Metadata {

    unsigned int num_lines;
    unsigned int num_columns;

    // Defines the origin of the image
    int line_offset;
    int column_offset;

    MAGIC_EXACT resolution;

    MAGIC_EXACT satellite_radius_km;
    MAGIC_EXACT full_disk_scan_min;

    // Selected sampling angle
    MAGIC_EXACT angular_sampling_rad;

    int wavelength;

    // Available sampling grids for this satellite
    std::unordered_map<int, MAGIC_EXACT> angular_sampling;

    // available wavelengths for this satellite
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
    metadata.full_disk_scan_min = config["navigation"]["full_disk_scan_min"].value_or(0.0);
    // ------------------------------------------------------------
    // Resolution factors
    // ------------------------------------------------------------

    if (const auto* grids =
            config["grids"].as_table())
    {
        for (const auto& [key, value] : *grids) {
            const int resolution = std::stoi(std::string(key));

            if (const auto* grid = value.as_table()) {
                if (const auto sampling =
                        (*grid)["angular_sampling_rad"].value<MAGIC_EXACT>()) {
                    metadata.angular_sampling[resolution] = *sampling;
                }
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