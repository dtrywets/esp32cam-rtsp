#pragma once

#include <string.h>
#include <sensor.h>

constexpr const char camera_rotations[][20] = {
	"Normal (0°)",
	"90° clockwise",
	"180°",
	"270° clockwise"};

inline int lookup_rotation_degrees(const char *name)
{
	if (strncmp(name, "90", 2) == 0)
		return 90;
	if (strncmp(name, "180", 3) == 0)
		return 180;
	if (strncmp(name, "270", 3) == 0)
		return 270;
	return 0;
}

inline bool is_preview_only_rotation(int degrees)
{
	return degrees == 90 || degrees == 270;
}

inline void apply_rotation_to_sensor(sensor_t *camera, int degrees, bool user_hmirror, bool user_vflip)
{
	switch (degrees)
	{
	case 180:
		camera->set_hmirror(camera, 1);
		camera->set_vflip(camera, 1);
		break;
	default:
		camera->set_hmirror(camera, user_hmirror);
		camera->set_vflip(camera, user_vflip);
		break;
	}
}
