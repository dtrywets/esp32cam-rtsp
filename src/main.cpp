#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <esp_wifi.h>
#include <soc/rtc_cntl_reg.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include <OV2640.h>
#include <ESPmDNS.h>
#include <rtsp_server.h>
#include <lookup_camera_effect.h>
#include <lookup_camera_frame_size.h>
#include <lookup_camera_rotation.h>
#include <lookup_camera_gainceiling.h>
#include <lookup_camera_wb_mode.h>
#include <format_duration.h>
#include <format_number.h>
#include <moustache.h>
#include <config_portal.h>
#include <settings.h>

// HTML files
extern const char index_html_min_start[] asm("_binary_html_index_min_html_start");

ConfigHtmlFormatProvider configHtmlFormatProvider;

auto param_group_device = iotwebconf::ParameterGroup("device", "Board");
auto param_status_led_blink = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("led")
								  .label(LABEL_STATUS_LED)
								  .defaultValue(DEFAULT_STATUS_LED_BLINK)
								  .build();

auto param_group_camera = iotwebconf::ParameterGroup("camera", "Camera");
auto param_frame_duration = iotwebconf::Builder<iotwebconf::UIntTParameter<unsigned long>>("fd")
								.label(LABEL_FRAME_DURATION)
								.defaultValue(DEFAULT_FRAME_DURATION)
								.min(50)
								.max(2000)
								.build();
auto param_frame_size = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(frame_sizes[0])>>("fs")
							.label(LABEL_FRAME_SIZE)
							.optionValues((const char *)&frame_sizes)
							.optionNames((const char *)&frame_sizes)
							.optionCount(sizeof(frame_sizes) / sizeof(frame_sizes[0]))
							.nameLength(sizeof(frame_sizes[0]))
							.defaultValue(DEFAULT_FRAME_SIZE)
							.build();
auto param_jpg_quality = iotwebconf::Builder<iotwebconf::UIntTParameter<byte>>("q")
							 .label(LABEL_JPG_QUALITY)
							 .defaultValue(DEFAULT_JPEG_QUALITY)
							 .min(4)
							 .max(63)
							 .build();
auto param_brightness = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("b")
							.label(LABEL_BRIGHTNESS)
							.defaultValue(DEFAULT_BRIGHTNESS)
							.min(-2)
							.max(2)
							.build();
auto param_contrast = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("c")
						  .label(LABEL_CONTRAST)
						  .defaultValue(DEFAULT_CONTRAST)
						  .min(-2)
						  .max(2)
						  .build();
auto param_saturation = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("s")
							.label(LABEL_SATURATION)
							.defaultValue(DEFAULT_SATURATION)
							.min(-2)
							.max(2)
							.build();
auto param_special_effect = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(camera_effects[0])>>("e")
								.label(LABEL_SPECIAL_EFFECT)
								.optionValues((const char *)&camera_effects)
								.optionNames((const char *)&camera_effects)
								.optionCount(sizeof(camera_effects) / sizeof(camera_effects[0]))
								.nameLength(sizeof(camera_effects[0]))
								.defaultValue(DEFAULT_EFFECT)
								.build();
auto param_whitebal = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("wb")
						  .label(LABEL_WHITEBAL)
						  .defaultValue(DEFAULT_WHITE_BALANCE)
						  .build();
auto param_awb_gain = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("awbg")
						  .label(LABEL_AWB_GAIN)
						  .defaultValue(DEFAULT_WHITE_BALANCE_GAIN)
						  .build();
auto param_wb_mode = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(camera_wb_modes[0])>>("wbm")
						 .label(LABEL_WB_MODE)
						 .optionValues((const char *)&camera_wb_modes)
						 .optionNames((const char *)&camera_wb_modes)
						 .optionCount(sizeof(camera_wb_modes) / sizeof(camera_wb_modes[0]))
						 .nameLength(sizeof(camera_wb_modes[0]))
						 .defaultValue(DEFAULT_WHITE_BALANCE_MODE)
						 .build();
auto param_exposure_ctrl = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("ec")
							   .label(LABEL_EXPOSURE_CTRL)
							   .defaultValue(DEFAULT_EXPOSURE_CONTROL)
							   .build();
auto param_aec2 = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("aec2")
					  .label(LABEL_AEC2)
					  .defaultValue(DEFAULT_AEC2)
					  .build();
auto param_ae_level = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("ael")
						  .label(LABEL_AE_LEVEL)
						  .defaultValue(DEFAULT_AE_LEVEL)
						  .min(-2)
						  .max(2)
						  .build();
auto param_aec_value = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("aecv")
						   .label(LABEL_AEC_VALUE)
						   .defaultValue(DEFAULT_AEC_VALUE)
						   .min(9)
						   .max(1200)
						   .build();
auto param_gain_ctrl = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("gc")
						   .label(LABEL_GAIN_CTRL)
						   .defaultValue(DEFAULT_GAIN_CONTROL)
						   .build();
auto param_agc_gain = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("agcg")
						  .label(LABEL_AGC_GAIN)
						  .defaultValue(DEFAULT_AGC_GAIN)
						  .min(0)
						  .max(30)
						  .build();
auto param_gain_ceiling = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(camera_gain_ceilings[0])>>("gcl")
							  .label(LABEL_GAIN_CEILING)
							  .optionValues((const char *)&camera_gain_ceilings)
							  .optionNames((const char *)&camera_gain_ceilings)
							  .optionCount(sizeof(camera_gain_ceilings) / sizeof(camera_gain_ceilings[0]))
							  .nameLength(sizeof(camera_gain_ceilings[0]))
							  .defaultValue(DEFAULT_GAIN_CEILING)
							  .build();
auto param_bpc = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("bpc")
					 .label(LABEL_BPC)
					 .defaultValue(DEFAULT_BPC)
					 .build();
auto param_wpc = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("wpc")
					 .label(LABEL_WPC)
					 .defaultValue(DEFAULT_WPC)
					 .build();
auto param_raw_gma = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("rg")
						 .label(LABEL_RAW_GMA)
						 .defaultValue(DEFAULT_RAW_GAMMA)
						 .build();
auto param_lenc = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("lenc")
					  .label(LABEL_LENC)
					  .defaultValue(DEFAULT_LENC)
					  .build();
auto param_hmirror = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("hm")
						 .label(LABEL_HMIRROR)
						 .defaultValue(DEFAULT_HORIZONTAL_MIRROR)
						 .build();
auto param_vflip = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("vm")
					   .label(LABEL_VFLIP)
					   .defaultValue(DEFAULT_VERTICAL_MIRROR)
					   .build();
auto param_rotation = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(camera_rotations[0])>>("rot")
						  .label(LABEL_ROTATION)
						  .optionValues((const char *)&camera_rotations)
						  .optionNames((const char *)&camera_rotations)
						  .optionCount(sizeof(camera_rotations) / sizeof(camera_rotations[0]))
						  .nameLength(sizeof(camera_rotations[0]))
						  .defaultValue(DEFAULT_ROTATION)
						  .build();
auto param_dcw = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("dcw")
					 .label(LABEL_DCW)
					 .defaultValue(DEFAULT_DCW)
					 .build();
auto param_colorbar = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("cb")
						  .label(LABEL_COLORBAR)
						  .defaultValue(DEFAULT_COLORBAR)
						  .build();

// Camera
OV2640 cam;
// DNS Server
DNSServer dnsServer;
// RTSP Server
std::unique_ptr<rtsp_server> camera_server;
// Web server
WebServer web_server(80);

auto thingName = String(WIFI_SSID) + "-" + String(ESP.getEfuseMac(), 16);
IotWebConf iotWebConf(thingName.c_str(), &dnsServer, &web_server, WIFI_PASSWORD, CONFIG_VERSION);

// Camera initialization result
esp_err_t camera_init_result;
static bool skip_config_saved_callback = false;
static bool ota_initialized = false;
static unsigned long last_camera_capture_ms = 0;
static String last_wifi_fail_text = "not connected yet";
static SemaphoreHandle_t camera_capture_mutex = nullptr;
static framesize_t active_frame_size = FRAMESIZE_INVALID;
static byte active_jpeg_quality = 0;

const char *wifi_disconnect_reason_text(uint8_t reason)
{
	switch (reason)
	{
	case WIFI_REASON_UNSPECIFIED:
		return "UNSPECIFIED";
	case WIFI_REASON_AUTH_EXPIRE:
		return "AUTH_EXPIRE";
	case WIFI_REASON_AUTH_LEAVE:
		return "AUTH_LEAVE";
	case WIFI_REASON_ASSOC_EXPIRE:
		return "ASSOC_EXPIRE";
	case WIFI_REASON_ASSOC_TOOMANY:
		return "ASSOC_TOOMANY";
	case WIFI_REASON_NOT_AUTHED:
		return "NOT_AUTHED";
	case WIFI_REASON_NOT_ASSOCED:
		return "NOT_ASSOCED";
	case WIFI_REASON_ASSOC_LEAVE:
		return "ASSOC_LEAVE";
	case WIFI_REASON_ASSOC_NOT_AUTHED:
		return "ASSOC_NOT_AUTHED";
	case WIFI_REASON_DISASSOC_PWRCAP_BAD:
		return "DISASSOC_PWRCAP_BAD";
	case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
		return "DISASSOC_SUPCHAN_BAD";
	case WIFI_REASON_IE_INVALID:
		return "IE_INVALID";
	case WIFI_REASON_MIC_FAILURE:
		return "MIC_FAILURE";
	case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
		return "4WAY_HANDSHAKE_TIMEOUT — wrong password?";
	case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
		return "GROUP_KEY_UPDATE_TIMEOUT";
	case WIFI_REASON_IE_IN_4WAY_DIFFERS:
		return "IE_IN_4WAY_DIFFERS";
	case WIFI_REASON_GROUP_CIPHER_INVALID:
		return "GROUP_CIPHER_INVALID";
	case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
		return "PAIRWISE_CIPHER_INVALID";
	case WIFI_REASON_AKMP_INVALID:
		return "AKMP_INVALID";
	case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
		return "UNSUPP_RSN_IE_VERSION";
	case WIFI_REASON_INVALID_RSN_IE_CAP:
		return "INVALID_RSN_IE_CAP";
	case WIFI_REASON_802_1X_AUTH_FAILED:
		return "802_1X_AUTH_FAILED — enterprise WiFi not supported";
	case WIFI_REASON_CIPHER_SUITE_REJECTED:
		return "CIPHER_SUITE_REJECTED";
	case WIFI_REASON_BEACON_TIMEOUT:
		return "BEACON_TIMEOUT — weak signal or AP lost";
	case WIFI_REASON_NO_AP_FOUND:
		return "NO_AP_FOUND — SSID not found (typo or 5 GHz only!)";
	case WIFI_REASON_AUTH_FAIL:
		return "AUTH_FAIL — wrong WiFi password";
	case WIFI_REASON_ASSOC_FAIL:
		return "ASSOC_FAIL";
	case WIFI_REASON_HANDSHAKE_TIMEOUT:
		return "HANDSHAKE_TIMEOUT — password or WPA3 issue";
	default:
		return "OTHER";
	}
}

void on_wifi_event(WiFiEvent_t event, WiFiEventInfo_t info)
{
	if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
	{
		const auto reason = info.wifi_sta_disconnected.reason;
		const auto *text = wifi_disconnect_reason_text(reason);
		last_wifi_fail_text = String(text) + " (" + String(reason) + ")";
		log_w("WiFi STA disconnected: %s", last_wifi_fail_text.c_str());
	}
	else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
	{
		last_wifi_fail_text = "connected";
	}
}

void stop_rtsp_server();
void mark_camera_frame_captured();
void setup_arduino_ota();

bool require_admin_auth()
{
	if (iotWebConf.getState() != iotwebconf::NetworkState::OnLine)
		return true;

	if (!web_server.authenticate(IOTWEBCONF_ADMIN_USER_NAME, iotWebConf.getApPasswordParameter()->valueBuffer))
	{
		web_server.requestAuthentication();
		return false;
	}
	return true;
}

void reset_wifi_configuration()
{
	log_i("Resetting WiFi configuration");
	stop_rtsp_server();

	auto *wifi_ssid = iotWebConf.getWifiSsidParameter()->valueBuffer;
	auto *wifi_password = iotWebConf.getWifiPasswordParameter()->valueBuffer;
	memset(wifi_ssid, 0, iotWebConf.getWifiSsidParameter()->getLength());
	memset(wifi_password, 0, iotWebConf.getWifiPasswordParameter()->getLength());
	strncpy(iotWebConf.getApTimeoutParameter()->valueBuffer, "180", iotWebConf.getApTimeoutParameter()->getLength());

	skip_config_saved_callback = true;
	iotWebConf.resetWifiAuthInfo();
	iotWebConf.saveConfig();
	skip_config_saved_callback = false;

	WiFi.disconnect(true);
	delay(250);
	ESP.restart();
}

void configure_wifi_for_streaming()
{
	WiFi.setSleep(false);
	WiFi.setAutoReconnect(true);
	esp_wifi_set_ps(WIFI_PS_NONE);
	WiFi.setTxPower(WIFI_POWER_19_5dBm);
	esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
}

void mark_camera_frame_captured()
{
	last_camera_capture_ms = millis();
}

unsigned long effective_frame_interval_ms()
{
	auto frame_size = lookup_frame_size(param_frame_size.value());
	auto interval = param_frame_duration.value();
	auto min_interval = recommended_frame_duration_ms(frame_size);
	return interval > min_interval ? interval : min_interval;
}

unsigned long browser_preview_interval_ms()
{
	auto interval = effective_frame_interval_ms();
	if (camera_server != nullptr && camera_server->num_connected() > 0)
		return interval * 2;
	return interval;
}

byte clamp_jpeg_quality(byte quality)
{
	// esp_camera uses 0-63 (lower = better quality).
	if (quality > 63)
		return 63;
	if (quality < 4)
		return 4;
	return quality;
}

void mark_active_camera_settings()
{
	active_frame_size = lookup_frame_size(param_frame_size.value());
	active_jpeg_quality = clamp_jpeg_quality(param_jpg_quality.value());
}

bool camera_settings_changed()
{
	return lookup_frame_size(param_frame_size.value()) != active_frame_size ||
		   clamp_jpeg_quality(param_jpg_quality.value()) != active_jpeg_quality;
}

bool capture_camera_frame(bool force_new = false, bool allow_new_capture = true)
{
	auto interval = effective_frame_interval_ms();

	if (!allow_new_capture)
		return last_camera_capture_ms != 0 && cam.getSize() > 0 && cam.getfb() != nullptr;

	if (!force_new && last_camera_capture_ms != 0 &&
		millis() - last_camera_capture_ms < interval &&
		cam.getSize() > 0 && cam.getfb() != nullptr)
		return true;

	if (camera_capture_mutex == nullptr ||
		xSemaphoreTake(camera_capture_mutex, pdMS_TO_TICKS(force_new ? 500 : 0)) != pdTRUE)
		return cam.getSize() > 0 && cam.getfb() != nullptr;

	if (!force_new && last_camera_capture_ms != 0 &&
		millis() - last_camera_capture_ms < interval &&
		cam.getSize() > 0 && cam.getfb() != nullptr)
	{
		xSemaphoreGive(camera_capture_mutex);
		return true;
	}

	cam.run();
	mark_camera_frame_captured();
	xSemaphoreGive(camera_capture_mutex);
	return cam.getfb() != nullptr && cam.getSize() > 0;
}

bool provide_rtsp_camera_frame(uint8_t **frame, size_t *len)
{
	if (!capture_camera_frame(false))
		return false;

	*frame = cam.getfb();
	*len = cam.getSize();
	return *frame != nullptr && *len > 0;
}

void stop_rtsp_server()
{
	camera_server.reset();
}

void apply_status_led_setting()
{
#ifdef USER_LED_GPIO
	if (param_status_led_blink.value())
		iotWebConf.enableBlink();
	else
	{
		iotWebConf.disableBlink();
		digitalWrite(USER_LED_GPIO, !USER_LED_ON_LEVEL);
	}
#endif
}

void setup_arduino_ota()
{
	if (ota_initialized || WiFi.status() != WL_CONNECTED)
		return;

	ArduinoOTA.setHostname(iotWebConf.getThingName());
	ArduinoOTA.setPassword(OTA_PASSWORD);

	ArduinoOTA.onStart([]()
					   {
						 stop_rtsp_server();
						 log_i("ArduinoOTA update started");
					   });
	ArduinoOTA.onEnd([]()
					 { log_i("ArduinoOTA update finished"); });
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
						  { log_i("ArduinoOTA progress: %u%%", (progress * 100) / total); });
	ArduinoOTA.onError([](ota_error_t error)
					   { log_e("ArduinoOTA error[%u]", error); });

	ArduinoOTA.begin();
	ota_initialized = true;
	log_i("ArduinoOTA ready (port 3232, password: %s)", OTA_PASSWORD);
}

void handle_update_get()
{
	log_v("handle_update_get");
	if (!require_admin_auth())
		return;

	web_server.send(200, "text/html",
					"<!DOCTYPE html><html><head><meta charset='utf-8'><title>Firmware update</title></head><body>"
					"<h1>Firmware update</h1>"
					"<p>Upload a <code>.bin</code> firmware file built for this board.</p>"
					"<form method='POST' action='/update' enctype='multipart/form-data'>"
					"<input type='file' name='firmware' accept='.bin,application/octet-stream' required>"
					"<button type='submit'>Upload and reboot</button>"
					"</form>"
					"<p><a href='/'>Back</a></p>"
					"</body></html>");
}

void handle_update_post()
{
	log_v("handle_update_post");
	web_server.sendHeader("Connection", "close");
	web_server.send(200, "text/plain", Update.hasError() ? "Update failed" : "Update OK, rebooting...");
	delay(500);
	ESP.restart();
}

void handle_update_upload()
{
	auto &upload = web_server.upload();

	if (upload.status == UPLOAD_FILE_START)
	{
		if (!require_admin_auth())
		{
			log_w("Web update rejected: authentication required");
			return;
		}

		log_i("Web update start: %s", upload.filename.c_str());
		stop_rtsp_server();
		if (!Update.begin(UPDATE_SIZE_UNKNOWN))
			Update.printError(Serial);
	}
	else if (upload.status == UPLOAD_FILE_WRITE)
	{
		if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
			Update.printError(Serial);
	}
	else if (upload.status == UPLOAD_FILE_END)
	{
		if (Update.end(true))
			log_i("Web update success: %u bytes", upload.totalSize);
		else
			Update.printError(Serial);
	}
	else if (upload.status == UPLOAD_FILE_ABORTED)
	{
		log_w("Web update aborted");
		Update.end();
	}
}

void handle_root()
{
  log_v("Handle root");
  // Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
    return;

  // Format hostname
  auto hostname = "esp32-" + WiFi.macAddress() + ".local";
  hostname.replace(":", "");
  hostname.toLowerCase();

  // Wifi Modes
  const char *wifi_modes[] = {"NULL", "STA", "AP", "STA+AP"};
  auto ipv4 = WiFi.getMode() == WIFI_MODE_AP ? WiFi.softAPIP() : WiFi.localIP();
  auto ipv6 = WiFi.getMode() == WIFI_MODE_AP ? WiFi.softAPIPv6() : WiFi.localIPv6();

  auto initResult = esp_err_to_name(camera_init_result);
  if (initResult == nullptr)
    initResult = "Unknown reason";

  moustache_variable_t substitutions[] = {
      // Version / CPU
      {"AppTitle", APP_TITLE},
      {"AppVersion", APP_VERSION},
      {"BoardType", BOARD_NAME},
      {"ThingName", iotWebConf.getThingName()},
      {"SDKVersion", ESP.getSdkVersion()},
      {"ChipModel", ESP.getChipModel()},
      {"ChipRevision", String(ESP.getChipRevision())},
      {"CpuFreqMHz", String(ESP.getCpuFreqMHz())},
      {"CpuCores", String(ESP.getChipCores())},
      {"FlashSize", format_memory(ESP.getFlashChipSize(), 0)},
      {"HeapSize", format_memory(ESP.getHeapSize())},
      {"PsRamSize", format_memory(ESP.getPsramSize(), 0)},
      // Diagnostics
      {"Uptime", String(format_duration(millis() / 1000))},
      {"FreeHeap", format_memory(ESP.getFreeHeap())},
      {"MaxAllocHeap", format_memory(ESP.getMaxAllocHeap())},
      {"NumRTSPSessions", camera_server != nullptr ? String(camera_server->num_connected()) : "RTSP server disabled"},
      {"NumRTSPStreaming", camera_server != nullptr ? String(camera_server->num_streaming()) : "0"},
      // Network
      {"HostName", hostname},
      {"MacAddress", WiFi.macAddress()},
      {"AccessPoint", WiFi.SSID()},
      {"SignalStrength", String(WiFi.RSSI())},
      {"WifiMode", wifi_modes[WiFi.getMode()]},
      {"IPv4", ipv4.toString()},
      {"IPv6", ipv6.toString()},
      {"SavedWifiSsid", String(iotWebConf.getWifiSsidParameter()->valueBuffer)},
      {"LastWifiError", last_wifi_fail_text},
      {"NetworkState.ApMode", String(iotWebConf.getState() == iotwebconf::NetworkState::ApMode)},
      {"NetworkState.OnLine", String(iotWebConf.getState() == iotwebconf::NetworkState::OnLine)},
      // Camera
      {"FrameSize", String(param_frame_size.value())},
      {"FrameDuration", String(param_frame_duration.value())},
      {"EffectiveFrameMs", String(effective_frame_interval_ms())},
      {"BrowserPreviewMs", String(browser_preview_interval_ms())},
      {"FrameFrequency", String(1000.0 / effective_frame_interval_ms(), 1)},
      {"JpegQuality", String(param_jpg_quality.value())},
      {"CameraInitialized", String(camera_init_result == ESP_OK)},
      {"CameraInitResult", String(camera_init_result)},
      {"CameraInitResultText", initResult},
      // Settings
      {"Brightness", String(param_brightness.value())},
      {"Contrast", String(param_contrast.value())},
      {"Saturation", String(param_saturation.value())},
      {"SpecialEffect", String(param_special_effect.value())},
      {"WhiteBal", String(param_whitebal.value())},
      {"AwbGain", String(param_awb_gain.value())},
      {"WbMode", String(param_wb_mode.value())},
      {"ExposureCtrl", String(param_exposure_ctrl.value())},
      {"Aec2", String(param_aec2.value())},
      {"AeLevel", String(param_ae_level.value())},
      {"AecValue", String(param_aec_value.value())},
      {"GainCtrl", String(param_gain_ctrl.value())},
      {"AgcGain", String(param_agc_gain.value())},
      {"GainCeiling", String(param_gain_ceiling.value())},
      {"Bpc", String(param_bpc.value())},
      {"Wpc", String(param_wpc.value())},
      {"RawGma", String(param_raw_gma.value())},
      {"Lenc", String(param_lenc.value())},
      {"HMirror", String(param_hmirror.value())},
      {"VFlip", String(param_vflip.value())},
      {"Rotation", String(param_rotation.value())},
      {"RotationDegrees", String(lookup_rotation_degrees(param_rotation.value()))},
      {"PreviewRotationDegrees", String(is_preview_only_rotation(lookup_rotation_degrees(param_rotation.value()))
                                         ? lookup_rotation_degrees(param_rotation.value())
                                         : 0)},
      {"Dcw", String(param_dcw.value())},
      {"ColorBar", String(param_colorbar.value())},
      // RTSP
      {"RtspPort", String(RTSP_PORT)}};

  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  auto html = moustache_render(index_html_min_start, substitutions);
  web_server.send(200, "text/html", html);
}

#ifdef FLASH_LED_GPIO
void handle_flash()
{
  log_v("handle_flash");
  // If no value present, use off, otherwise convert v to integer. Depends on analog resolution for max value
  auto v = web_server.hasArg("v") ? web_server.arg("v").toInt() : 0;
  // If conversion fails, v = 0
  analogWrite(FLASH_LED_GPIO, v);

  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.send(200);
}
#endif

void handle_snapshot()
{
  log_v("handle_snapshot");
  if (camera_init_result != ESP_OK)
  {
    web_server.send(404, "text/plain", "Camera is not initialized");
    return;
  }

  const bool rtsp_streaming = camera_server != nullptr && camera_server->num_streaming() > 0;
  if (!capture_camera_frame(false, !rtsp_streaming))
  {
    web_server.send(404, "text/plain", rtsp_streaming
                                       ? "Preview paused while RTSP is active"
                                       : "Unable to obtain frame buffer from the camera");
    return;
  }

  auto fb_len = cam.getSize();
  auto fb = (const char *)cam.getfb();

  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.setContentLength(fb_len);
  web_server.send(200, "image/jpeg", "");
  web_server.sendContent(fb, fb_len);
}

#define STREAM_CONTENT_BOUNDARY "123456789000000000000987654321"

void handle_stream()
{
  log_v("handle_stream");
  if (camera_init_result != ESP_OK)
  {
    web_server.send(404, "text/plain", "Camera is not initialized");
    return;
  }

  log_v("starting streaming");
  configure_wifi_for_streaming();
  if (camera_server)
    camera_server->set_capture_paused(true);

  // Blocks further handling of HTTP server until stopped, but keeps WiFi/RTSP alive.
  char size_buf[12];
  auto client = web_server.client();
  client.setNoDelay(true);
  client.write("HTTP/1.1 200 OK\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "Cache-Control: no-cache, no-store, must-revalidate\r\n"
               "Connection: close\r\n"
               "Content-Type: multipart/x-mixed-replace; boundary=" STREAM_CONTENT_BOUNDARY "\r\n\r\n");
  client.flush();

  auto frame_interval = effective_frame_interval_ms();
  unsigned long last_frame_ms = 0;
  while (client.connected())
  {
    iotWebConf.doLoop();
    if (camera_server)
      camera_server->doLoop();

    auto now = millis();
    if (last_frame_ms != 0 && now - last_frame_ms < frame_interval)
    {
      delay(1);
      continue;
    }
    last_frame_ms = now;

    if (!capture_camera_frame(true))
    {
      log_w("HTTP stream: camera frame capture failed");
      delay(frame_interval);
      continue;
    }

    client.write("--" STREAM_CONTENT_BOUNDARY "\r\n");
    client.write("Content-Type: image/jpeg\r\nContent-Length: ");
    sprintf(size_buf, "%u\r\n\r\n", (unsigned)cam.getSize());
    client.write(size_buf);
    client.write(cam.getfb(), cam.getSize());
    client.flush();
  }

  log_v("client disconnected");
  client.stop();
  if (camera_server)
    camera_server->set_capture_paused(false);
  log_v("stopped streaming");
}

void handle_restart()
{
	log_v("handle_restart");
	if (!require_admin_auth())
		return;

	WiFi.disconnect(false, true);
	ESP.restart();
}

void handle_wifi_reset()
{
	log_v("handle_wifi_reset");
	if (!require_admin_auth())
		return;

	web_server.send(200, "text/html",
					"<html><body><h1>WiFi reset</h1>"
					"<p>Stored WiFi credentials were cleared. The device will restart "
					"and open a configuration hotspot.</p>"
					"<p>Connect to the WiFi network named <b>" +
						String(iotWebConf.getThingName()) +
						"</b>, then open <a href='http://192.168.4.1/'>http://192.168.4.1</a> "
						"to choose a new access point.</p></body></html>");

	delay(500);
	reset_wifi_configuration();
}

void handle_wifi_reconnect()
{
	log_v("handle_wifi_reconnect");
	if (!require_admin_auth())
		return;

	web_server.send(200, "text/html",
					"<html><body><h1>Reconnecting WiFi</h1>"
					"<p>Restarting with saved credentials…</p></body></html>");
	delay(500);
	iotWebConf.forceApMode(false);
	ESP.restart();
}

esp_err_t initialize_camera()
{
  log_v("initialize_camera");

  log_i("Frame size: %s", param_frame_size.value());
  auto frame_size = lookup_frame_size(param_frame_size.value());
  auto jpeg_quality = clamp_jpeg_quality(param_jpg_quality.value());
  log_i("JPEG quality: %d", jpeg_quality);
  log_i("Frame duration: %d ms", param_frame_duration.value());
  const camera_config_t camera_config = {
      .pin_pwdn = CAMERA_CONFIG_PIN_PWDN,         // GPIO pin for camera power down line
      .pin_reset = CAMERA_CONFIG_PIN_RESET,       // GPIO pin for camera reset line
      .pin_xclk = CAMERA_CONFIG_PIN_XCLK,         // GPIO pin for camera XCLK line
      .pin_sccb_sda = CAMERA_CONFIG_PIN_SCCB_SDA, // GPIO pin for camera SDA line
      .pin_sccb_scl = CAMERA_CONFIG_PIN_SCCB_SCL, // GPIO pin for camera SCL line
      .pin_d7 = CAMERA_CONFIG_PIN_Y9,             // GPIO pin for camera D7 line
      .pin_d6 = CAMERA_CONFIG_PIN_Y8,             // GPIO pin for camera D6 line
      .pin_d5 = CAMERA_CONFIG_PIN_Y7,             // GPIO pin for camera D5 line
      .pin_d4 = CAMERA_CONFIG_PIN_Y6,             // GPIO pin for camera D4 line
      .pin_d3 = CAMERA_CONFIG_PIN_Y5,             // GPIO pin for camera D3 line
      .pin_d2 = CAMERA_CONFIG_PIN_Y4,             // GPIO pin for camera D2 line
      .pin_d1 = CAMERA_CONFIG_PIN_Y3,             // GPIO pin for camera D1 line
      .pin_d0 = CAMERA_CONFIG_PIN_Y2,             // GPIO pin for camera D0 line
      .pin_vsync = CAMERA_CONFIG_PIN_VSYNC,       // GPIO pin for camera VSYNC line
      .pin_href = CAMERA_CONFIG_PIN_HREF,         // GPIO pin for camera HREF line
      .pin_pclk = CAMERA_CONFIG_PIN_PCLK,         // GPIO pin for camera PCLK line
      .xclk_freq_hz = CAMERA_CONFIG_CLK_FREQ_HZ,  // Frequency of XCLK signal, in Hz. EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
      .ledc_timer = CAMERA_CONFIG_LEDC_TIMER,     // LEDC timer to be used for generating XCLK
      .ledc_channel = CAMERA_CONFIG_LEDC_CHANNEL, // LEDC channel to be used for generating XCLK
      .pixel_format = PIXFORMAT_JPEG,             // Format of the pixel data: PIXFORMAT_ + YUV422|GRAYSCALE|RGB565|JPEG
      .frame_size = frame_size,                   // Size of the output image: FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
      .jpeg_quality = jpeg_quality,               // Quality of JPEG output. 0-63 lower means higher quality
      .fb_count = CAMERA_CONFIG_FB_COUNT,         // Number of frame buffers to be allocated. If more than one, then each frame will be acquired (double speed)
      .fb_location = CAMERA_CONFIG_FB_LOCATION,   // The location where the frame buffer will be allocated
      .grab_mode = CAMERA_GRAB_LATEST,            // When buffers should be filled
#if CONFIG_CAMERA_CONVERTER_ENABLED
      conv_mode = CONV_DISABLE, // RGB<->YUV Conversion mode
#endif
      .sccb_i2c_port = SCCB_I2C_PORT // If pin_sccb_sda is -1, use the already configured I2C bus by number
  };

  return cam.init(camera_config);
}

void update_camera_settings()
{
  auto camera = esp_camera_sensor_get();
  if (camera == nullptr)
  {
    log_e("Unable to get camera sensor");
    return;
  }

  camera->set_brightness(camera, param_brightness.value());
  camera->set_contrast(camera, param_contrast.value());
  camera->set_saturation(camera, param_saturation.value());
  camera->set_special_effect(camera, lookup_camera_effect(param_special_effect.value()));
  camera->set_whitebal(camera, param_whitebal.value());
  camera->set_awb_gain(camera, param_awb_gain.value());
  camera->set_wb_mode(camera, lookup_camera_wb_mode(param_wb_mode.value()));
  camera->set_exposure_ctrl(camera, param_exposure_ctrl.value());
  camera->set_aec2(camera, param_aec2.value());
  camera->set_ae_level(camera, param_ae_level.value());
  camera->set_aec_value(camera, param_aec_value.value());
  camera->set_gain_ctrl(camera, param_gain_ctrl.value());
  camera->set_agc_gain(camera, param_agc_gain.value());
  camera->set_gainceiling(camera, lookup_camera_gainceiling(param_gain_ceiling.value()));
  camera->set_bpc(camera, param_bpc.value());
  camera->set_wpc(camera, param_wpc.value());
  camera->set_raw_gma(camera, param_raw_gma.value());
  camera->set_lenc(camera, param_lenc.value());
  apply_rotation_to_sensor(
	  camera,
	  lookup_rotation_degrees(param_rotation.value()),
	  param_hmirror.value(),
	  param_vflip.value());
  camera->set_dcw(camera, param_dcw.value());
  camera->set_colorbar(camera, param_colorbar.value());
}

esp_err_t reinitialize_camera()
{
  log_i("Re-initializing camera for %s (JPEG %d)", param_frame_size.value(), clamp_jpeg_quality(param_jpg_quality.value()));
  stop_rtsp_server();
  esp_camera_deinit();
  last_camera_capture_ms = 0;

  for (auto i = 0; i < 3; i++)
  {
    camera_init_result = initialize_camera();
    if (camera_init_result == ESP_OK)
    {
      update_camera_settings();
      mark_active_camera_settings();
      return ESP_OK;
    }

    esp_camera_deinit();
    log_e("Camera re-init failed: 0x%x", camera_init_result);
    delay(500);
  }

  return camera_init_result;
}

void start_rtsp_server()
{
  log_v("start_rtsp_server");
  stop_rtsp_server();
  auto interval = effective_frame_interval_ms();
  log_i("RTSP frame interval: %lu ms (%s)", interval, param_frame_size.value());
  camera_server = std::unique_ptr<rtsp_server>(new rtsp_server(cam, interval, RTSP_PORT));
  camera_server->set_frame_provider(provide_rtsp_camera_frame);
  // Add RTSP service to mDNS
  // HTTP is already set by iotWebConf
  MDNS.addService("rtsp", "tcp", RTSP_PORT);
}

void on_connected()
{
  log_v("on_connected");
  configure_wifi_for_streaming();
  setup_arduino_ota();
  // Start the RTSP Server if initialized
  if (camera_init_result == ESP_OK)
    start_rtsp_server();
  else
    log_e("Not starting RTSP server: camera not initialized");
}

void on_config_saved()
{
  log_v("on_config_saved");
  if (skip_config_saved_callback)
    return;

  apply_status_led_setting();
  update_camera_settings();

  if (camera_settings_changed())
  {
    log_i("Resolution or JPEG quality changed — re-initializing camera");
    reinitialize_camera();
  }

  const auto state = iotWebConf.getState();
  if (state == iotwebconf::NetworkState::OnLine)
  {
    if (camera_init_result == ESP_OK)
      start_rtsp_server();
    return;
  }

  // Saved WiFi credentials while in setup hotspot — must restart to connect.
  if (state == iotwebconf::NetworkState::ApMode ||
      state == iotwebconf::NetworkState::NotConfigured)
  {
    log_i("WiFi settings saved in AP mode, restarting to connect");
    iotWebConf.forceApMode(false);
    delay(500);
    ESP.restart();
  }
}

void setup()
{
  // Disable brownout
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFi.onEvent(on_wifi_event);
  configure_wifi_for_streaming();
#ifdef CAMERA_POWER_GPIO
  pinMode(CAMERA_POWER_GPIO, OUTPUT);
  digitalWrite(CAMERA_POWER_GPIO, CAMERA_POWER_ON_LEVEL);
#endif

#ifdef USER_LED_GPIO
  pinMode(USER_LED_GPIO, OUTPUT);
  digitalWrite(USER_LED_GPIO, !USER_LED_ON_LEVEL);
#endif

#ifdef FLASH_LED_GPIO
  pinMode(FLASH_LED_GPIO, OUTPUT);
  // Set resolution to 8 bits
  analogWriteResolution(8);
  // Turn flash led off
  analogWrite(FLASH_LED_GPIO, 0);
#endif

#ifdef ARDUINO_USB_CDC_ON_BOOT
  // Delay for USB to connect/settle
  delay(5000);
#endif

  log_i("Core debug level: %d", CORE_DEBUG_LEVEL);
  log_i("CPU Freq: %d Mhz, %d core(s)", getCpuFrequencyMhz(), ESP.getChipCores());
  log_i("Free heap: %d bytes", ESP.getFreeHeap());
  log_i("SDK version: %s", ESP.getSdkVersion());
  log_i("Board: %s", BOARD_NAME);
  log_i("Starting " APP_TITLE "...");

  camera_capture_mutex = xSemaphoreCreateMutex();

  if (CAMERA_CONFIG_FB_LOCATION == CAMERA_FB_IN_PSRAM && !psramInit())
    log_e("Failed to initialize PSRAM");

  param_group_device.addItem(&param_status_led_blink);
  iotWebConf.addParameterGroup(&param_group_device);

  param_group_camera.addItem(&param_frame_duration);
  param_group_camera.addItem(&param_frame_size);
  param_group_camera.addItem(&param_jpg_quality);
  param_group_camera.addItem(&param_brightness);
  param_group_camera.addItem(&param_contrast);
  param_group_camera.addItem(&param_saturation);
  param_group_camera.addItem(&param_special_effect);
  param_group_camera.addItem(&param_whitebal);
  param_group_camera.addItem(&param_awb_gain);
  param_group_camera.addItem(&param_wb_mode);
  param_group_camera.addItem(&param_exposure_ctrl);
  param_group_camera.addItem(&param_aec2);
  param_group_camera.addItem(&param_ae_level);
  param_group_camera.addItem(&param_aec_value);
  param_group_camera.addItem(&param_gain_ctrl);
  param_group_camera.addItem(&param_agc_gain);
  param_group_camera.addItem(&param_gain_ceiling);
  param_group_camera.addItem(&param_bpc);
  param_group_camera.addItem(&param_wpc);
  param_group_camera.addItem(&param_raw_gma);
  param_group_camera.addItem(&param_lenc);
  param_group_camera.addItem(&param_hmirror);
  param_group_camera.addItem(&param_vflip);
  param_group_camera.addItem(&param_rotation);
  param_group_camera.addItem(&param_dcw);
  param_group_camera.addItem(&param_colorbar);
  iotWebConf.addParameterGroup(&param_group_camera);

  iotWebConf.getApTimeoutParameter()->visible = true;
  iotWebConf.setHtmlFormatProvider(&configHtmlFormatProvider);
  iotWebConf.setConfigSavedCallback(on_config_saved);
  iotWebConf.setWifiConnectionCallback(on_connected);
  iotWebConf.setWifiConnectionTimeoutMs(60000);
  iotWebConf.setWifiConnectionFailedHandler([]() -> iotwebconf::WifiAuthInfo *
											{
											  log_w("WiFi connection failed — AP stays open until timeout or new config save");
											  return nullptr;
											});
#ifdef USER_LED_GPIO
  iotWebConf.setStatusPin(USER_LED_GPIO, USER_LED_ON_LEVEL);
#endif
  iotWebConf.init();
  apply_status_led_setting();

  // Try to initialize 3 times
  for (auto i = 0; i < 3; i++)
  {
    camera_init_result = initialize_camera();
    if (camera_init_result == ESP_OK)
    {
      update_camera_settings();
      mark_active_camera_settings();
      break;
    }

    esp_camera_deinit();
    log_e("Failed to initialize camera. Error: 0x%0x. Frame size: %s, frame rate: %d ms, jpeg quality: %d", camera_init_result, param_frame_size.value(), param_frame_duration.value(), param_jpg_quality.value());
    delay(500);
  }

  // Set up required URL handlers on the web server
  web_server.on("/", HTTP_GET, handle_root);
  web_server.on("/config", []
                { iotWebConf.handleConfig(); });
  // Camera snapshot
  web_server.on("/snapshot", HTTP_GET, handle_snapshot);
  // Camera stream
  web_server.on("/stream", HTTP_GET, handle_stream);
#ifdef FLASH_LED_GPIO
  // Flash led
  web_server.on("/flash", HTTP_GET, handle_flash);
#endif
  // ESP restart
  web_server.on("/restart", HTTP_GET, handle_restart);
  // Clear WiFi credentials and reopen configuration hotspot
  web_server.on("/wifi-reset", HTTP_GET, handle_wifi_reset);
  web_server.on("/wifi-reconnect", HTTP_GET, handle_wifi_reconnect);
  // Firmware update (web upload)
  web_server.on("/update", HTTP_GET, handle_update_get);
  web_server.on("/update", HTTP_POST, handle_update_post, handle_update_upload);
  web_server.onNotFound([]()
                        { iotWebConf.handleNotFound(); });
}

void loop()
{
  iotWebConf.doLoop();

  if (ota_initialized)
    ArduinoOTA.handle();

  if (camera_server)
    camera_server->doLoop();
}
