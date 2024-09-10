/*
    ------------------------------------------------------------------

    This file is part of the Tracking plugin for the Open Ephys GUI
    Written by:

    Alessio Buccino     alessiob@ifi.uio.no
    Mikkel Lepperod
    Svenn-Arne Dragly

    Center for Integrated Neuroplasticity CINPLA
    Department of Biosciences
    University of Oslo
    Norway

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TRACKINGDATA_H
#define TRACKINGDATA_H

#include <ProcessorHeaders.h>

struct TrackingPosition {
    float x;
    float y;
    float width;
    float height;
};

struct TrackingData {
    uint64 timestamp;
    TrackingPosition position;
    friend std::ostream &operator<<(std::ostream &stream, const TrackingData &td){
        stream << "x: " << td.position.x << std::endl;
        stream << "y: " << td.position.y << std::endl;
        stream << "width: " << td.position.width << std::endl;
        stream << "height: " << td.position.height << std::endl;
        return stream;
    };
};

struct TrackingSources
{
    unsigned int eventIndex;
    unsigned int sourceId;
    float x_pos;
    float y_pos;
    float width;
    float height;
    String name;
    String color;
    bool positionInsideACircle;
    friend std::ostream &operator<<(std::ostream &stream, const TrackingSources &ts){
        stream << "name: " << ts.name << std::endl;
        stream << "Color: " << ts.color << std::endl;
        stream << "x: " << ts.x_pos << std::endl;
        stream << "y: " << ts.y_pos << std::endl;
        stream << "width: " << ts.width << std::endl;
        stream << "height: " << ts.height << std::endl;
        return stream;
    };
};

#endif // TRACKINGDATA_H
