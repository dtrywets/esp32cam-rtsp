#pragma once

#include <IotWebConf.h>

// Hint texts use HTML in labels (rendered by IotWebConf).
#define HINT_SUFFIX "<br><span class='hint'>"

#define LABEL_STATUS_LED \
	"Red status LED blink" HINT_SUFFIX \
	"Off = LED stays dark. On = blinks for WiFi state (fast=AP, slow=connected).</span>"

#define LABEL_FRAME_DURATION \
	"Frame interval (ms)" HINT_SUFFIX \
	"Higher = slower stream, less CPU/WiFi load. Lower = smoother video, more load. " \
	"SVGA: use 500–1000 ms (1–2 FPS). VGA: 200–333 ms. Range 50–2000 ms.</span>"

#define LABEL_FRAME_SIZE \
	"Resolution" HINT_SUFFIX \
	"SVGA (800x600): ESP32 limit ~2 FPS. DCW on, JPEG 18–25, frame 500+ ms. Close status page when using RTSP/Frigate.</span>"

#define LABEL_JPG_QUALITY \
	"JPEG quality" HINT_SUFFIX \
	"Lower number = better image, larger files. Higher = worse image, less bandwidth. " \
	"Camera range 4–63. Recommended 10–20 with PSRAM.</span>"

#define LABEL_BRIGHTNESS \
	"Brightness" HINT_SUFFIX \
	"Range -2 (dark) to +2 (bright). 0 = default.</span>"

#define LABEL_CONTRAST \
	"Contrast" HINT_SUFFIX \
	"Range -2 (low) to +2 (high). 0 = default.</span>"

#define LABEL_SATURATION \
	"Saturation" HINT_SUFFIX \
	"Range -2 (less colour) to +2 (more colour). 0 = default.</span>"

#define LABEL_SPECIAL_EFFECT \
	"Colour effect" HINT_SUFFIX \
	"Image filter. Use Normal for live video.</span>"

#define LABEL_WHITEBAL \
	"Auto white balance" HINT_SUFFIX \
	"On = camera adjusts colour temperature automatically.</span>"

#define LABEL_AWB_GAIN \
	"AWB gain" HINT_SUFFIX \
	"Fine-tunes auto white balance. Usually leave on.</span>"

#define LABEL_WB_MODE \
	"White balance mode" HINT_SUFFIX \
	"Auto for most scenes. Sunny/Cloudy for outdoor tuning.</span>"

#define LABEL_EXPOSURE_CTRL \
	"Auto exposure" HINT_SUFFIX \
	"On = automatic shutter time. Off = use manual exposure value below.</span>"

#define LABEL_AEC2 \
	"Auto exposure (DSP)" HINT_SUFFIX \
	"Software exposure control. Usually leave on.</span>"

#define LABEL_AE_LEVEL \
	"Auto exposure level" HINT_SUFFIX \
	"Range -2 (darker) to +2 (brighter). 0 = neutral.</span>"

#define LABEL_AEC_VALUE \
	"Manual exposure" HINT_SUFFIX \
	"Only if auto exposure is off. Higher = brighter, more blur. Range 9–1200.</span>"

#define LABEL_GAIN_CTRL \
	"Auto gain (AGC)" HINT_SUFFIX \
	"On = boosts brightness in dark scenes (may add noise).</span>"

#define LABEL_AGC_GAIN \
	"Manual AGC gain" HINT_SUFFIX \
	"Only if auto gain is off. Range 0 (min) to 30 (max).</span>"

#define LABEL_GAIN_CEILING \
	"Max auto gain" HINT_SUFFIX \
	"Limits amplification in low light. 2X = mild, 128X = very bright/noisy.</span>"

#define LABEL_BPC \
	"Black pixel fix" HINT_SUFFIX \
	"Corrects dead dark pixels. Off recommended unless spots visible.</span>"

#define LABEL_WPC \
	"White pixel fix" HINT_SUFFIX \
	"Corrects bright stuck pixels. On recommended.</span>"

#define LABEL_RAW_GMA \
	"Gamma correction" HINT_SUFFIX \
	"Improves mid-tones. Usually leave on.</span>"

#define LABEL_LENC \
	"Lens shading" HINT_SUFFIX \
	"Reduces dark corners from lens. Usually leave on.</span>"

#define LABEL_HMIRROR \
	"Mirror horizontally" HINT_SUFFIX \
	"Flips image left/right. Overridden when rotation is 180°.</span>"

#define LABEL_VFLIP \
	"Flip vertically" HINT_SUFFIX \
	"Flips image top/bottom. Overridden when rotation is 180°.</span>"

#define LABEL_ROTATION \
	"Image rotation" HINT_SUFFIX \
	"180° = sensor flip (RTSP + preview). 90°/270° = browser preview only — use Frigate/go2rtc rotate for RTSP.</span>"

#define LABEL_DCW \
	"Downsize (DCW)" HINT_SUFFIX \
	"On = scales in hardware (faster, recommended). Off = full sensor readout.</span>"

#define LABEL_COLORBAR \
	"Test colour bar" HINT_SUFFIX \
	"On = test pattern instead of camera (for debugging only).</span>"

class ConfigHtmlFormatProvider : public iotwebconf::HtmlFormatProvider
{
protected:
	String getStyleInner() override
	{
		return HtmlFormatProvider::getStyleInner() +
			   ".hint{display:block;font-size:.85em;color:#555;font-weight:normal;margin-top:4px;line-height:1.35}"
			   "label{line-height:1.3}"
			   "fieldset{margin-bottom:1.2em}"
			   "legend{font-weight:bold}"
			   ".portal-intro{max-width:520px;margin:0 auto 1em;text-align:left;font-size:.9em;color:#444}";
	}

	String getBodyInner() override
	{
		return "<p class='portal-intro'>Camera values marked with a range: <b>low/min</b> = less effect or load, "
			   "<b>high/max</b> = stronger effect or more load. After changing resolution or quality, use "
			   "<a href='/restart'>restart</a>.<br>"
			   "<b>WiFi:</b> ESP32 needs <b>2.4 GHz</b> (not 5 GHz only). After saving WiFi, the device restarts automatically.</p>" +
			   HtmlFormatProvider::getBodyInner();
	}
};
