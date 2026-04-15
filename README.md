# neoswift

A fork of [swift pilot client](https://swift-project.org/) for FSD-compatible flight simulation networks — built for networks that aren't VATSIM.

swift is a capable, cross-platform pilot client, but in practice it only works well for VATSIM: VATSIM endpoints are hardcoded, authentication is tied to proprietary closed-source components, and third-party networks get a degraded experience with no path to first-class support. neoswift changes that.

> **IMPORTANT**: neoswift is **NOT** a VATSIM-approved client and is not intended for use on the VATSIM network.
>
> neoswift removes VATSIM proprietary components(`vatsimauth.dll`). While it retains VATSIMAuth challenge support for use with third-party networks that implement the same handshake, **connecting to VATSIM is not guaranteed and may result in a ban**.
>
> If you want to connect to VATSIM, use the [original swift client](https://swift-project.org/) instead.

## What's different

### VATSIM proprietary components removed
- `vatsimauth` — VATSIM's closed-source client/server verification library(and a HWID collector) — has been replaced an reimplementation. This means neoswift can connect to networks that implement the VATSIMAuth challenge handshake without shipping a proprietary binary.
- VATSIM-specific hardcoded endpoints, authentication flows, and UI have been removed.

### Network auto-discovery
Add any compatible network with just a domain name, and enjoy the same convenience as connecting to VATSIM.

No more manual field-by-field setup or long setup insstructions, neoswift fetches `https://your-network.com/.well-known/fsd-configuration.json` and configures everything automatically — FSD protocol version, authentication method, server list, METAR source, and even AFV voice server

### Full configurability
neoswift enables full configurability to network operators with its auto-discovery feature.

Every FSD protocol behavior that swift exposes is now configurable per-network, and fully automated with auto-discovery:
- Protocol revision
- Authentication method: JWT token endpoint or legacy plaintext password
- VATSIMAuth challenge behaviors
- Per-direction flags: aircraft parts, interim positions, ground flag, visual positions
- EuroScope sim data, ICAO equipment field format, and more
- Custom AFV-compatible voice server, or disable AFV

And even more:
- Provide a dynamic updating server list dropdown, just like VATSIM, for your user to select from.

## For network operators

Want your network to work with neoswift? Add a `fsd-configuration.json` to your domain's `.well-known/` path and your users get one-click setup.

→ [Network Operator Guide](docs/network-operator-guide.md)

## For users whose network isn't supported yet

Check [awesome-neoswift](https://github.com/skylitefly/awesome-neoswift) for community-maintained configuration files for networks that haven't added official neoswift support.

## Building

neoswift is a fork of swift and builds with the same toolchain. See [BUILDING.md](BUILDING.md).

To enable VATSIMAuth challenge support (required for networks that implement the handshake):
```cmake
cmake -DSWIFT_VATSIM_SUPPORT=ON ...
```

## License

GPL-3.0, inherited from swift pilot client. See [LICENSE](LICENSE).

neoswift is not affiliated with, endorsed by, or approved by VATSIM.

neoswift is not a VATSIM-approved client and is not intended to connect to the VATSIM network.
