#pragma once

#include <stddef.h>

/*
 * Runtime configuration of the steamclient facade.
 *
 * Read once from <game>/config/loader.cfg:
 *
 *   # 0 = do not query masters (Internet tab stays empty).
 *   # Favorites and LAN are unaffected. Default is 1.
 *   internet = 1
 *
 *   # masters to query for the Internet server list
 *   masters = 10.0.0.5:27010, 10.0.0.6:27010
 *
 * An empty or missing "masters" list, or internet = 0, means that no master
 * is queried at all.  No master is baked into the binary - every deployment
 * names its own.  CMake -DVELLUM_NO_SERVER_BROWSER=ON compiles the Internet
 * query out entirely (same effect as internet = 0, but at build time).
 */
#define VELLUM_CFG_MAX_MASTERS 8

/* Directory of the game: the folder steamclient.dll was loaded from. */
void Vellum_GameDir(char *dir, size_t dirSize);

/* Parse <game>/config/loader.cfg.  Safe to call more than once. */
void Vellum_ConfigLoad(void);

/* 0 = never query masters, even if a list is present. */
int Vellum_ConfigInternetEnabled(void);

/* Masters named in the config, in order.  Zero means "no master". */
int Vellum_ConfigMasterCount(void);
const char *Vellum_ConfigMaster(int index);
