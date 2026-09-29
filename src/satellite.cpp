#include "../headers/satellite.hpp"

namespace Satellite {

    // Gets the exact obs time for a given line 
    // As the satellite takes some time to take the entire picture
    // TODO: The old code may have been wrapping the time incorrectly. Check this. 
    // TODO: Get this directly from the filename?
    MAGIC_EXACT calcObsTime(unsigned int line, Metadata info, DateTime time) {
        
        MAGIC_EXACT observation_time_utc = time.hour + time.minute / 60.0 +
            MAGIC_EXACT(line) * (info.full_disk_scan_min / info.num_lines / 60.0);
            
        if (observation_time_utc < 0.0) return 24.0 - observation_time_utc;

        return observation_time_utc;

        // This seems more correct but does not produce same results as specmagic now
        // return fmod(observation_time_utc + 24.0, 24.0);

    }


    // Depending on how image origin is defined this may be needed
    int flipVertical(int line, int height) {
        return height - line;
    }

    int flipHorizontal(int col, int width) {
	    return width - col;
	}


    /**
     * @brief Convert geographic coordinates (lat, lon) to image pixel coordinates.
     *
     * This function implements the official geostationary projection used by MTG
     * (Meteosat Third Generation). It maps a geodetic Earth position (latitude,
     * longitude) to line/column indices in the MTG image.
     *
     * The calculation:
     *  - uses a WGS84-like Earth ellipsoid
     *  - accounts for satellite height and Earth curvature
     *  - checks satellite visibility (Earth limb)
     *
     * @param lat_rad   Latitude in radians
     * @param lon_rad   Longitude in radians (relative to satellite sub-point)
     * @param[out] col  Resulting column index (0-based)
     * @param[out] line Resulting line index (0-based)
     *
     * @throws std::runtime_error if the point is not visible or outside the image
     */
    void geo2Image(MAGIC_EXACT lat_rad, MAGIC_EXACT lon_rad, Metadata& metadata,
        unsigned int& col, unsigned int& line) {

        // -------------------------------------------------
        // Convert geodetic latitude -> geocentric latitude
        // -------------------------------------------------

        MAGIC_EXACT geocentric_lat = std::atan(RPE2 * std::tan(lat_rad));

        MAGIC_EXACT cos_lat = std::cos(geocentric_lat);
        MAGIC_EXACT sin_lat = std::sin(geocentric_lat);

        // --------------------------------------------
        // Earth radius at given latitude (ellipsoid)
        // --------------------------------------------

        MAGIC_EXACT earth_radius = EARTH_POLAR_RADIUS_KM / std::sqrt(1.0 - EPSI2 * cos_lat * cos_lat);

        // --------------------------------------------
        // Vector from satellite to Earth surface point
        // --------------------------------------------
        MAGIC_EXACT satellite_radius = metadata.satellite_radius_km;

        MAGIC_EXACT r1 = satellite_radius - earth_radius * cos_lat * std::cos(lon_rad);
        MAGIC_EXACT r2 = - earth_radius * cos_lat * std::sin(lon_rad);
        MAGIC_EXACT r3 = earth_radius * sin_lat;

        MAGIC_EXACT range = std::sqrt(r1 * r1 + r2 * r2 + r3 * r3);

        // --------------------------------------------
        // Viewing angles (satellite scan angles)
        // --------------------------------------------

        MAGIC_EXACT alpha = std::atan2(r2, r1);   // east-west scan angle
        MAGIC_EXACT beta  = std::asin(r3 / range); // north-south scan angle

        // --------------------------------------------
        // Visibility check (Earth limb test)
        // --------------------------------------------

        MAGIC_EXACT visibility = metadata.satellite_radius_km *
                std::cos(alpha) * std::cos(beta)
            - range *
                (std::pow(std::cos(beta), 2.0) +
                RPE2 * std::pow(std::sin(beta), 2.0));

        // This wont fire just if a pixel is in the dark or similar
        // but checks whether we are simulating over a grid that is not covered
        if (visibility < 0.0) {
            throw std::runtime_error(
                "Geographic point is not visible from satellite");
        }

        MAGIC_EXACT column = metadata.column_offset - alpha / metadata.angular_sampling_rad;
        MAGIC_EXACT line_val = metadata.line_offset - beta / metadata.angular_sampling_rad;

        // --------------------------------------------
        // Round to nearest pixel
        // --------------------------------------------

        col  = static_cast<uint>(std::lround(column));
        line = static_cast<uint>(std::lround(line_val));

        // --------------------------------------------
        // Image bounds check
        // --------------------------------------------
        if (col == 0 || col >= metadata.num_columns ||
            line == 0 || line >= metadata.num_lines) {
            throw std::runtime_error(
                "Mapped pixel lies outside image bounds");
        }
    }

}   // Namespace Satellite

