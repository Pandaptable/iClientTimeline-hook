#pragma once

#include <nlohmann/json.hpp>
#include <ixwebsocket/IXWebSocket.h>

#include <atomic>
#include <array>
#include <cstdint>
#include <print>
#include <string>
#include <vector>

using json = nlohmann::json;

// Debug-only logging — define before any use in this header or main.cpp
#ifndef NDEBUG
#define LOG(...) std::println(__VA_ARGS__)
#else
#define LOG(...) ((void)0)
#endif

// ---------------------------------------------------------------------------
// Portable SHA-256 (no Windows SDK / BCrypt needed)
// ---------------------------------------------------------------------------

namespace detail
{

static constexpr uint32_t kK[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z)  { return (x & y) ^ (~x & z); }
inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t ep0(uint32_t x) { return rotr(x, 2)  ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t ep1(uint32_t x) { return rotr(x, 6)  ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t sig0(uint32_t x){ return rotr(x, 7)  ^ rotr(x, 18) ^ (x >> 3);  }
inline uint32_t sig1(uint32_t x){ return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

} // namespace detail

static std::vector<uint8_t> Sha256(const std::string &input)
{
	uint32_t h[8] = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
	};

	std::vector<uint8_t> msg(input.begin(), input.end());
	uint64_t bitLen = static_cast<uint64_t>(msg.size()) * 8;
	msg.push_back(0x80);
	while (msg.size() % 64 != 56)
		msg.push_back(0x00);
	for (int i = 7; i >= 0; --i)
		msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));

	for (size_t chunk = 0; chunk < msg.size(); chunk += 64)
	{
		uint32_t w[64];
		for (int i = 0; i < 16; ++i)
			w[i] = (uint32_t(msg[chunk + i*4])     << 24) |
			       (uint32_t(msg[chunk + i*4 + 1]) << 16) |
			       (uint32_t(msg[chunk + i*4 + 2]) <<  8) |
			       (uint32_t(msg[chunk + i*4 + 3]));
		for (int i = 16; i < 64; ++i)
			w[i] = detail::sig1(w[i-2]) + w[i-7] + detail::sig0(w[i-15]) + w[i-16];

		uint32_t a = h[0], b = h[1], c = h[2], d = h[3],
		         e = h[4], f = h[5], g = h[6], hh = h[7];

		for (int i = 0; i < 64; ++i)
		{
			uint32_t t1 = hh + detail::ep1(e) + detail::ch(e, f, g) + detail::kK[i] + w[i];
			uint32_t t2 = detail::ep0(a) + detail::maj(a, b, c);
			hh = g; g = f; f = e; e = d + t1;
			d  = c; c = b; b = a; a = t1 + t2;
		}

		h[0] += a; h[1] += b; h[2] += c; h[3] += d;
		h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
	}

	std::vector<uint8_t> digest(32);
	for (int i = 0; i < 8; ++i)
	{
		digest[i*4]     = (h[i] >> 24) & 0xFF;
		digest[i*4 + 1] = (h[i] >> 16) & 0xFF;
		digest[i*4 + 2] = (h[i] >>  8) & 0xFF;
		digest[i*4 + 3] =  h[i]        & 0xFF;
	}
	return digest;
}

// ---------------------------------------------------------------------------
// Portable Base64 encoder
// ---------------------------------------------------------------------------

static std::string Base64Encode(const std::vector<uint8_t> &data)
{
	static constexpr char kTable[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	out.reserve(((data.size() + 2) / 3) * 4);
	for (size_t i = 0; i < data.size(); i += 3)
	{
		uint32_t b = static_cast<uint32_t>(data[i]) << 16;
		if (i + 1 < data.size()) b |= static_cast<uint32_t>(data[i + 1]) << 8;
		if (i + 2 < data.size()) b |= static_cast<uint32_t>(data[i + 2]);
		out += kTable[(b >> 18) & 0x3F];
		out += kTable[(b >> 12) & 0x3F];
		out += (i + 1 < data.size()) ? kTable[(b >> 6) & 0x3F] : '=';
		out += (i + 2 < data.size()) ? kTable[ b       & 0x3F] : '=';
	}
	return out;
}

// ---------------------------------------------------------------------------
// OBS WebSocket v5 auth helper
//   base64( SHA256( base64( SHA256(password + salt) ) + challenge ) )
// ---------------------------------------------------------------------------

static std::string ObsComputeAuth(const std::string &password,
                                  const std::string &salt,
                                  const std::string &challenge)
{
	std::string base64Secret = Base64Encode(Sha256(password + salt));
	return Base64Encode(Sha256(base64Secret + challenge));
}

// ---------------------------------------------------------------------------
// OBS WebSocket client
// ---------------------------------------------------------------------------

class ObsWebSocket
{
public:
	explicit ObsWebSocket(std::string host     = "127.0.0.1",
	                      uint16_t    port     = 4455,
	                      std::string password = "")
	    : m_host(std::move(host))
	    , m_port(port)
	    , m_password(std::move(password))
	{
	}

	~ObsWebSocket() { Disconnect(); }

	ObsWebSocket(const ObsWebSocket &)            = delete;
	ObsWebSocket &operator=(const ObsWebSocket &) = delete;

	void Connect()
	{
		std::string url = "ws://" + m_host + ":" + std::to_string(m_port);
		LOG("[OBS] Connecting to {}\n", url);

		m_ws.setUrl(url);
		m_ws.disableAutomaticReconnection();

		m_ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg)
		{
			switch (msg->type)
			{
			case ix::WebSocketMessageType::Open:
				LOG("[OBS] Connection opened\n");
				break;

			case ix::WebSocketMessageType::Close:
				LOG("[OBS] Connection closed: {} {}\n",
				    msg->closeInfo.code, msg->closeInfo.reason);
				m_identified = false;
				break;

			case ix::WebSocketMessageType::Error:
				LOG("[OBS] Error: {}\n", msg->errorInfo.reason);
				break;

			case ix::WebSocketMessageType::Message:
				OnMessage(msg->str);
				break;

			default:
				break;
			}
		});

		m_ws.start();
	}

	void Disconnect()
	{
		m_ws.stop();
		m_identified = false;
	}

	bool IsIdentified() const { return m_identified; }

	// Broadcast a Steam Timeline event to all OBS WebSocket clients (op 6 BroadcastCustomEvent)
	void Broadcast(const std::string &type, json data = json::object())
	{
		SendRequest("BroadcastCustomEvent", "steam_timeline",
		    {{"eventData", {{"source", "SteamTimeline"}, {"type", type}, {"data", data}}}});
	}

	// Send a raw OBS request (op 6)
	void SendRequest(const std::string &requestType,
	                 const std::string &requestId = "1",
	                 json               data      = json::object())
	{
		if (!m_identified)
			return;

		json msg = {
		    {"op", 6},
		    {"d",  {
		        {"requestType", requestType},
		        {"requestId",   requestId},
		        {"requestData", data},
		    }},
		};
		m_ws.sendText(msg.dump());
	}

private:
	void OnMessage(const std::string &raw)
	{
		json msg;
		try { msg = json::parse(raw); }
		catch (const std::exception &e)
		{
			LOG("[OBS] JSON parse error: {}\n", e.what());
			return;
		}

		int  op = msg.value("op", -1);
		auto &d = msg["d"];

		switch (op)
		{
		case 0: // Hello
		{
			LOG("[OBS] Hello – rpcVersion={}\n", d.value("rpcVersion", 0));

			json identifyData = {{"rpcVersion", 1}};

			if (d.contains("authentication"))
			{
				std::string challenge = d["authentication"]["challenge"];
				std::string salt      = d["authentication"]["salt"];
				LOG("[OBS] Auth required – computing authentication string\n");
				identifyData["authentication"] = ObsComputeAuth(m_password, salt, challenge);
			}
			else
			{
				LOG("[OBS] No auth required\n");
			}

			m_ws.sendText(json{{"op", 1}, {"d", identifyData}}.dump());
			break;
		}

		case 2: // Identified
			LOG("[OBS] Identified! negotiatedRpcVersion={}\n",
			    d.value("negotiatedRpcVersion", 0));
			m_identified = true;
			break;

		case 5: // Event
			LOG("[OBS] Event: {} (intent={})\n",
			    d.value("eventType",   "?"),
			    d.value("eventIntent", 0));
			break;

		case 7: // RequestResponse
			LOG("[OBS] RequestResponse: type={} id={} status={}\n",
			    d.value("requestType", "?"),
			    d.value("requestId",   "?"),
			    d["requestStatus"].value("result", false) ? "ok" : "error");
			break;

		default:
			LOG("[OBS] Unknown opcode: {}\n", op);
			break;
		}
	}

	std::string       m_host;
	uint16_t          m_port;
	std::string       m_password;
	ix::WebSocket     m_ws;
	std::atomic<bool> m_identified{false};
};
