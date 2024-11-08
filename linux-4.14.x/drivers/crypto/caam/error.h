/* SPDX-License-Identifier: GPL-2.0 */
/*
 * CAAM Error Reporting code header
 *
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 */

#ifndef CAAM_ERROR_H
#define CAAM_ERROR_H
#define CAAM_ERROR_STR_MAX 302

void caam_strstatus(struct device *dev, u32 status, bool qi_v2);

#define caam_jr_strstatus(jrdev, status) caam_strstatus(jrdev, status, false)
#define caam_qi2_strstatus(qidev, status) caam_strstatus(qidev, status, true)

void caam_dump_sg(const char *level, const char *prefix_str, int prefix_type,
		  int rowsize, int groupsize, struct scatterlist *sg,
		  size_t tlen, bool ascii);

#ifdef	CONFIG_WG_PLATFORM // WG:JB Return Linux errors
#define	linux_error(x) (((x & 0xF00000FF) == 0x4000001C) ? -ETIMEDOUT : -EIO)
#define	LINUX_ERROR(x) (unlikely(x) ? linux_error(x) : 0)
#endif

#endif /* CAAM_ERROR_H */
