#pragma once

#include <functional>
#include <list>
#include <WiFiServer.h>
#include <ESPmDNS.h>
#include <OV2640.h>
#include <CRtspSession.h>
#include <arduino-timer.h>

class rtsp_server : public WiFiServer
{
public:
	rtsp_server(OV2640 &cam, unsigned long interval, int port = 554);

	void doLoop();

	size_t num_connected();

	size_t num_streaming();

	void set_capture_paused(bool paused) { capture_paused_ = paused; }

	void set_on_frame_captured(std::function<void()> callback)
	{
		on_frame_captured_ = std::move(callback);
	}

	using frame_provider_t = std::function<bool(uint8_t **frame, size_t *len)>;
	void set_frame_provider(frame_provider_t callback)
	{
		frame_provider_ = std::move(callback);
	}

private:
	struct rtsp_client
	{
	public:
		rtsp_client(const WiFiClient &client,  OV2640 &cam);

		WiFiClient wifi_client;
		// Streamer for UDP/TCP based RTP transport
		std::shared_ptr<CStreamer> streamer;
		// RTSP session and state
		std::shared_ptr<CRtspSession> session;
	};

	OV2640 cam_;
	bool capture_paused_ = false;
	std::function<void()> on_frame_captured_;
	frame_provider_t frame_provider_;
	std::list<std::unique_ptr<rtsp_client>> clients_;
	uintptr_t task_;
	Timer<> timer_;

	static bool client_handler(void *);
};