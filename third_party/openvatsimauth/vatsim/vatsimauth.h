// SPDX-FileCopyrightText: Copyright (C) OpenVatsimAuth Contributors
// SPDX-License-Identifier: MIT
// OpenVatsimAuth – C API header

#ifndef OPENVATSIMAUTH_H
#define OPENVATSIMAUTH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vatsim_auth;
typedef struct vatsim_auth vatsim_auth;

vatsim_auth *vatsim_auth_create(uint16_t clientId, const char *privateKey);
void vatsim_auth_destroy(vatsim_auth *auth);
uint16_t vatsim_auth_get_client_id(const vatsim_auth *auth);
void vatsim_auth_set_initial_challenge(vatsim_auth *auth, const char *initialChallenge);
void vatsim_auth_generate_response(vatsim_auth *auth, const char *challenge, char *response);
void vatsim_auth_generate_challenge(const vatsim_auth *auth, char *challenge);
void vatsim_get_system_unique_id(char *systemId);

#ifdef __cplusplus
}
#endif

#endif // OPENVATSIMAUTH_H
