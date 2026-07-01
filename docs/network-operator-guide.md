# Network Operator Guide

This guide is for operators who want to make their FSD network discoverable by this client.

---

## Overview

This client uses a DNS-based auto-discovery mechanism. When a user types your domain name the client fetches:

```
https://{domain}/.well-known/fsd-configuration.json
```

That JSON file tells the client everything it needs to know: which FSD protocol to speak, how to authenticate, where to fetch the server list, where to fetch network data and METARs, and whether voice (AFV) is available.

The client then fetches the server list from the `servers_url` you specify, builds the server dropdown, and the user can connect.

---

## Quick Start (Classic FSD, Plain Auth)

Minimal working configuration for a classic FSD server with plain-text passwords:

```json
{
  "version": 1,
  "network": {
    "name": "MyNetwork",
    "description": "My FSD network"
  },
  "fsd": {
    "protocol": "classic",
    "challenge": false,
    "auth": "plain",
    "auth_url": null,
    "servers_url": "https://data.mynetwork.example/servers.json",
    "load_balancing_url": null,
    "text_codec": "UTF-8",
    "aircraft_parts":    { "send": true,  "receive": true  },
    "interim_positions": { "send": false, "receive": false },
    "gnd_flag":          { "send": true,  "receive": true  },
    "send_visual_positions": false,
    "send_fpl_icao_equipment": false,
    "receive_euroscope_simdata": false,
    "force_3_letter_airline_icao": false
  },
  "data": {
    "network_data_url": "https://data.mynetwork.example/network-data.json",
    "metar_url": "https://metar.mynetwork.example/metar.php",
    "data_polling_interval_sec": 120,
    "max_range_nm": -1
  },
  "voice": {
    "enabled": false,
    "api_url": null,
    "map_url": null
  }
}
```

Host this file at `https://yourdomain.example/.well-known/fsd-configuration.json` with:
- `Content-Type: application/json`
- `Access-Control-Allow-Origin: *`
- HTTPS required

---

## fsd-configuration.json Reference

### `version`

Always `1`. Future versions will be backwards-incompatible.

---

### `network` section

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Short name shown in the network dropdown (e.g. `"MyNetwork"`) |
| `description` | string | Longer description shown in the UI |

---

### `fsd` section

#### `fsd.protocol`

Controls the FSD protocol revision number announced during login.

| Value | Protocol revision | Extra behaviour |
|-------|-------------------|-----------------|
| `"classic"` | 9 | Standard FSD. After TCP connect, client sends `$AP` login directly. |
| `"vatsim-velocity"` | 101 | Waits for server `$DI` (FsdIdentification) before login. Enables visual position updates (`$SB`). ATIS is delivered via a dedicated message type instead of falling back to private message. |

Use `"classic"` unless you are running a server that specifically speaks protocol 101.

---

#### `fsd.challenge`

`true` | `false` (default: `false`)

Controls whether the VATSIMAuth challenge/response handshake (`$ID`/`$ZC`/`$ZR` messages) is enabled. This is **independent** of `fsd.auth`.

- `false`: No challenge. Simple connection, suitable for almost all networks.
- `true`: Enables the VATSIMAuth HMAC challenge handshake. **Requires:**
  - Your FSD server implements the server side of the VATSIMAuth protocol.
  - The client is compiled with `SWIFT_VATSIM_SUPPORT=ON` (i.e., the VATSIMAuth library must be available at build time).

Most networks should set this to `false`.

---

#### `fsd.auth`

`"plain"` | `"jwt"` (default: `"plain"`)

Controls how the password field of the `$AP` login message is populated.

| Value | Behaviour |
|-------|-----------|
| `"plain"` | The user's password is sent directly in the `$AP` message. |
| `"jwt"` | The client first POSTs `{"cid": "...", "password": "..."}` to `fsd.auth_url`, receives a JWT token, and uses that token as the password in `$AP`. |

For `"jwt"`, set `auth_url` to your token endpoint. The endpoint must respond with:

```json
{ "success": true, "token": "<jwt>", "error_msg": null }
```

or on failure:

```json
{ "success": false, "token": null, "error_msg": "Invalid credentials" }
```

---

#### `fsd.servers_url`

URL to a JSON file listing your FSD servers. The format is compatible with the VATSIM servers endpoint:

```json
[
  {
    "id": "server1",
    "hostname_or_ip": "fsd.mynetwork.example",
    "location": "Frankfurt, Germany",
    "name": "EU-WEST",
    "clients_connection_allowed": true,
    "port": 6809
  }
]
```

The `port` field defaults to `6809` if omitted. Entries with `clients_connection_allowed: false` are excluded from the dropdown.

---

#### `fsd.load_balancing_url`

Optional. If set, and the selected server name is `"AUTOMATIC"`, the client performs an HTTP GET to this URL and uses the returned IP address as the actual connection target. Set to `null` to disable.

---

#### `fsd.text_codec`

Character encoding for FSD messages. Common values: `"UTF-8"`, `"ISO-8859-1"`. Defaults to `"UTF-8"`.

---

#### `fsd.reconnect`

Optional. Controls automatic reconnect after an unexpected FSD socket disconnect. It does not run after the user manually disconnects or after the server kicks the client.

Default is no reconnect:

```json
"reconnect": {
  "enabled": false,
  "max_attempts": 0,
  "initial_delay_sec": 5,
  "backoff_multiplier": 2.0,
  "max_delay_sec": 60,
  "append_attempt_to_callsign": false
}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | bool | `false` | Enable reconnect attempts for network-style disconnects. |
| `max_attempts` | int | `0` | Maximum attempts. `0` means disabled, even if `enabled` is true. |
| `initial_delay_sec` | int | `5` | Delay before the first reconnect attempt. |
| `backoff_multiplier` | number | `2.0` | Multiplier for each later attempt. Values below `1.0` are treated as `1.0`. |
| `max_delay_sec` | int | `60` | Maximum delay cap for any attempt. |
| `append_attempt_to_callsign` | bool | `false` | If true, append the attempt number directly to the original callsign. For example, `SKY123` reconnects as `SKY1231`, then `SKY1232`. No hyphen is added. |

Use `append_attempt_to_callsign` only if your server can temporarily keep stale sessions and rejects a new login with the same callsign.

---

#### FSD flags

These flags control which optional FSD message types are active.

**Nested send/receive pairs** (both directions independently configurable):

| Field | Description |
|-------|-------------|
| `aircraft_parts` | `$SFSD` aircraft parts messages (gear, flaps, lights, etc.) |
| `interim_positions` | Fast/interim position update messages |
| `gnd_flag` | Ground flag in position updates |

Each takes `{"send": bool, "receive": bool}`.

**Single-direction flags** (flat JSON):

| Field | Direction | Description |
|-------|-----------|-------------|
| `send_visual_positions` | outbound | Visual position updates (`$SB`) used by protocol 101 |
| `send_fpl_icao_equipment` | outbound | Send flight plan equipment in ICAO format instead of FAA |
| `receive_euroscope_simdata` | inbound | Accept EuroScope SIMDATA packets |
| `force_3_letter_airline_icao` | outbound | Force 3-letter airline ICAO codes in all messages |

---

### `data` section

| Field | Type | Description |
|-------|------|-------------|
| `network_data_url` | string | URL to fetch online pilots and controllers. Format compatible with VATSIM `vatsim-data.json`. |
| `metar_url` | string | METAR endpoint. Queried as `{metar_url}?id=ICAO` for a single airport or `{metar_url}?id=all` for all METARs. Returns plain METAR text. |
| `data_polling_interval_sec` | int | How often the client polls `network_data_url` (seconds). Default: `120`. |
| `max_range_nm` | int | Maximum visibility range in nautical miles. `-1` = unlimited. Default: `-1`. |

**`network_data_url` minimum format:**

```json
{
  "pilots": [
    {
      "cid": 12345,
      "name": "John Doe",
      "callsign": "SKY123",
      "server": "EU-WEST",
      "pilot_rating": 1,
      "latitude": 51.477,
      "longitude": -0.461,
      "altitude": 35000,
      "groundspeed": 480,
      "transponder": "1234",
      "heading": 270,
      "qnh_i_hg": 29.92,
      "qnh_mb": 1013,
      "flight_plan": null,
      "logon_time": "2024-01-01T00:00:00Z",
      "last_updated": "2024-01-01T12:00:00Z"
    }
  ],
  "controllers": [],
  "atis": [],
  "servers": [],
  "facilities": [],
  "ratings": [],
  "pilot_ratings": []
}
```

---

### `voice` section

| Field | Type | Description |
|-------|------|-------------|
| `enabled` | bool | Whether voice (AFV) is available on this network. |
| `api_url` | string or null | Base URL for the AFV REST API. Required if `enabled: true`. |
| `map_url` | string or null | URL for the AFV map (online callsign positions). |

**AFV REST API** must implement at minimum:
- `POST {api_url}/api/v1/auth` — exchange CID/password for a voice JWT
- `GET {api_url}/api/v1/network/online/callsigns` — list of online callsigns with transceiver positions (for the AFV map)
- UDP connection endpoint for actual audio

The AFV protocol is publicly documented. If you do not plan to offer voice, set `"enabled": false` and the client will not attempt any voice connection.

---

## Server List Format

The file at `fsd.servers_url` must be a JSON array:

```json
[
  {
    "id": "unique-id",
    "hostname_or_ip": "fsd1.example.com",
    "location": "New York, USA",
    "name": "US-EAST",
    "clients_connection_allowed": true,
    "port": 6809
  },
  {
    "id": "unique-id-2",
    "hostname_or_ip": "fsd2.example.com",
    "location": "London, UK",
    "name": "EU-WEST",
    "clients_connection_allowed": true,
    "port": 6809
  }
]
```

Servers with `"clients_connection_allowed": false` are hidden from the dropdown.

---

## Configuration Examples

### Example 1: Classic FSD, Plain Auth (Minimal)

```json
{
  "version": 1,
  "network": { "name": "MyNet", "description": "Classic FSD" },
  "fsd": {
    "protocol": "classic", "challenge": false, "auth": "plain",
    "auth_url": null, "servers_url": "https://data.mynet.example/servers.json",
    "load_balancing_url": null, "text_codec": "UTF-8",
    "aircraft_parts":    { "send": true,  "receive": true  },
    "interim_positions": { "send": false, "receive": false },
    "gnd_flag":          { "send": true,  "receive": true  },
    "send_visual_positions": false, "send_fpl_icao_equipment": false,
    "receive_euroscope_simdata": false, "force_3_letter_airline_icao": false
  },
  "data": {
    "network_data_url": "https://data.mynet.example/network-data.json",
    "metar_url": "https://metar.mynet.example/metar.php",
    "data_polling_interval_sec": 120, "max_range_nm": -1
  },
  "voice": { "enabled": false, "api_url": null, "map_url": null }
}
```

### Example 2: JWT Auth (No Challenge)

```json
{
  "version": 1,
  "network": { "name": "SecureNet", "description": "JWT auth" },
  "fsd": {
    "protocol": "classic", "challenge": false, "auth": "jwt",
    "auth_url": "https://auth.securemet.example/api/fsd-jwt",
    "servers_url": "https://data.securenet.example/servers.json",
    "load_balancing_url": null, "text_codec": "UTF-8",
    "aircraft_parts":    { "send": true,  "receive": true  },
    "interim_positions": { "send": false, "receive": false },
    "gnd_flag":          { "send": true,  "receive": true  },
    "send_visual_positions": false, "send_fpl_icao_equipment": false,
    "receive_euroscope_simdata": false, "force_3_letter_airline_icao": false
  },
  "data": {
    "network_data_url": "https://data.securenet.example/network-data.json",
    "metar_url": "https://metar.securenet.example/metar.php",
    "data_polling_interval_sec": 60, "max_range_nm": -1
  },
  "voice": { "enabled": false, "api_url": null, "map_url": null }
}
```

### Example 3: Network with Voice (AFV)

```json
{
  "version": 1,
  "network": { "name": "VoiceNet", "description": "AFV voice enabled" },
  "fsd": {
    "protocol": "classic", "challenge": false, "auth": "plain",
    "auth_url": null, "servers_url": "https://data.voicenet.example/servers.json",
    "load_balancing_url": null, "text_codec": "UTF-8",
    "aircraft_parts":    { "send": true,  "receive": true  },
    "interim_positions": { "send": false, "receive": false },
    "gnd_flag":          { "send": true,  "receive": true  },
    "send_visual_positions": false, "send_fpl_icao_equipment": false,
    "receive_euroscope_simdata": false, "force_3_letter_airline_icao": false
  },
  "data": {
    "network_data_url": "https://data.voicenet.example/network-data.json",
    "metar_url": "https://metar.voicenet.example/metar.php",
    "data_polling_interval_sec": 120, "max_range_nm": -1
  },
  "voice": {
    "enabled": true,
    "api_url": "https://voice.voicenet.example",
    "map_url": "https://voice.voicenet.example"
  }
}
```

---

## Hosting Requirements

1. **HTTPS only.** The client rejects `http://` discovery URLs.
2. **CORS headers.** Your `.well-known/fsd-configuration.json` must include:
   ```
   Access-Control-Allow-Origin: *
   Content-Type: application/json
   ```
3. **Availability.** The discovery endpoint is fetched at startup and cached locally. If it is temporarily unavailable, the client uses its cached copy and retries in the background.

---

## Troubleshooting

| Symptom | Likely cause |
|---------|-------------|
| "Discovery failed" | The `.well-known/fsd-configuration.json` URL returned an error or invalid JSON. Check HTTPS cert, CORS headers, and JSON syntax. |
| Server list empty | `fsd.servers_url` is unreachable, or all entries have `clients_connection_allowed: false`. |
| Connection hangs silently | `fsd.protocol` is `"vatsim-velocity"` but your server doesn't send `$DI`. Switch to `"classic"`. |
| Auth fails immediately | `fsd.auth` is `"jwt"` but `auth_url` is unreachable or returns an unexpected format. |
| No voice even with `enabled: true` | `api_url` is empty or the AFV REST API is not responding correctly. |
| METAR always empty | `metar_url` doesn't respond to `?id=all` query parameter. |
