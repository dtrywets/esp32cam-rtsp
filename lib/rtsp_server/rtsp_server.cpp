#include "rtsp_server.h"
#include <esp32-hal-log.h>
#include <OV2640Streamer.h>

namespace
{
	// streamFrame() is protected in CStreamer; expose it for shared frame broadcast.
	class RtspFrameStreamer : public OV2640Streamer
	{
	public:
		RtspFrameStreamer(SOCKET client, OV2640 &cam)
			: OV2640Streamer(client, cam)
		{
		}

		void sendFrame(uint8_t *data, size_t len, uint32_t curMsec)
		{
			if (data != nullptr && len > 0)
				streamFrame(data, len, curMsec);
		}
	};
}

// URI: e.g. rtsp://192.168.178.27:554/mjpeg/1
rtsp_server::rtsp_server(OV2640 &cam, unsigned long interval, int port /*= 554*/)
	: WiFiServer(port), cam_(cam)
{
	log_i("Starting RTSP server");
	WiFiServer::begin();
	timer_.every(interval, client_handler, this);
}

size_t rtsp_server::num_connected()
{
	return clients_.size();
}

size_t rtsp_server::num_streaming()
{
	size_t count = 0;
	for (const auto &client : clients_)
		if (client->session->m_streaming && !client->session->m_stopped)
			count++;
	return count;
}

void rtsp_server::doLoop()
{
	timer_.tick();
}

rtsp_server::rtsp_client::rtsp_client(const WiFiClient &client, OV2640 &cam)
{
	wifi_client = client;
	wifi_client.setNoDelay(true);
	streamer = std::shared_ptr<RtspFrameStreamer>(new RtspFrameStreamer(&wifi_client, cam));
	session = std::shared_ptr<CRtspSession>(new CRtspSession(&wifi_client, streamer.get()));
}

bool rtsp_server::client_handler(void *arg)
{
	auto self = static_cast<rtsp_server *>(arg);
	// Check if a client wants to connect
	auto new_client = self->accept();
	if (new_client)
		self->clients_.push_back(std::unique_ptr<rtsp_client>(new rtsp_client(new_client, self->cam_)));

	auto now = millis();
	bool any_streaming = false;

	for (const auto &client : self->clients_)
	{
		client->session->handleRequests(0);
		if (client->session->m_streaming && !client->session->m_stopped)
			any_streaming = true;
	}

	// Capture one frame and fan out to all active RTSP sessions.
	if (any_streaming && !self->capture_paused_ && self->frame_provider_)
	{
		uint8_t *frame = nullptr;
		size_t frame_size = 0;
		if (!self->frame_provider_(&frame, &frame_size))
			frame = nullptr;

		if (frame != nullptr && frame_size > 0)
		{
			for (const auto &client : self->clients_)
			{
				if (!client->session->m_streaming || client->session->m_stopped)
					continue;

				if (!client->wifi_client.connected())
				{
					client->session->m_stopped = true;
					continue;
				}

				auto streamer = static_cast<RtspFrameStreamer *>(client->streamer.get());
				streamer->sendFrame(frame, frame_size, now);
			}
		}
		else
			log_w("RTSP: camera frame capture failed");
	}

	self->clients_.remove_if([](std::unique_ptr<rtsp_client> const &c)
							 { return c->session->m_stopped; });

	return true;
}
