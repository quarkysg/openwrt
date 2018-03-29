/* Copyright (c) 2013, 2015, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *
 */

#include <nss_crypto_hlos.h>
#include <nss_api_if.h>
#include <nss_crypto.h>
#include <nss_crypto_if.h>
#include <nss_crypto_hw.h>
#include <nss_crypto_ctrl.h>
#include <nss_crypto_dbg.h>

struct nss_crypto_ctrl gbl_crypto_ctrl = {0};

extern struct nss_crypto_drv_ctx gbl_ctx;

#define NSS_CRYPTO_SESSION_FREE_DELAY_TICKS  msecs_to_jiffies(NSS_CRYPTO_SESSION_FREE_TIMEOUT_SEC * 1000)
#define NSS_CRYPTO_SIZE_KB(x) ((x) >> 10)

/*
 * Standard initialization vector for SHA-1, source: FIPS 180-2
 */
static const uint32_t fips_sha1_iv[NSS_CRYPTO_AUTH_IV_REGS] = {
	0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};

/*
 * Standard initialization vector for SHA-256, source: FIPS 180-2
 */
static uint32_t fips_sha256_iv[NSS_CRYPTO_AUTH_IV_REGS] = {
	0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
	0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

/*
 * NULL IV
 */
static const uint32_t null_iv[NSS_CRYPTO_AUTH_IV_REGS] = {0};

/*
 * NULL keys
 */
static const uint8_t null_ckey[NSS_CRYPTO_CKEY_SZ] = {0};
static const uint8_t null_akey[NSS_CRYPTO_AKEY_SZ] = {0};

/*
 * nss_crypto_mem_realloc
 * 	Allocate the memory to accommodate present & previous instances.
 * 	If instance is not present earlier then the memory will be allocated
 * 	for the same.
 *
 * 	NOTE: Older memory will be freed up and this shall not be called
 * 	from atomic context.
 */
void *nss_crypto_mem_realloc(void *src, size_t src_len, size_t dst_len)
{
	void *dst = NULL;

	/*
	 * Allocate the memory to accommodate present & previous
	 * instances
	 *
	 * NOTE: Currently it is assumed that the memory requirements
	 * can be addressed by contiguos allocations
	 */
	dst = kzalloc(dst_len, GFP_KERNEL);
	if (dst == NULL) {
		nss_crypto_err("Unable to allocate memory\n");
		return NULL;
	}

	/*
	 * Check if it is first allocation
	 */
	if ((src == NULL) || (src_len == 0)) {
		return dst;
	}

	/*
	 * Copy the earlier allocated memory to newly allocated memory
	 */
	memcpy(dst, src, src_len);

	/*
	 * Free_up the earlier allocation
	 */
	kfree(src);

	return dst;
}

/*
 * nss_crypto_clear_cblk()
 * 	this updates CMD block mask to indicate valid bits to write in the registers
 */
static inline void nss_crypto_update_cblk_mask(struct nss_crypto_bam_cmd *cmd, bool clr)
{
	cmd->mask = clr ? 0 : CRYPTO_MASK_ALL;
}

/*
 * nss_crypto_write_cblk()
 * 	load CMD block with data
 *
 * it will load
 * - crypto register offsets
 * - crypto register values
 */
static inline void nss_crypto_write_cblk(struct nss_crypto_bam_cmd *cmd, uint32_t addr, uint32_t value)
{
	cmd->addr = CRYPTO_CMD_ADDR(addr);
	cmd->value = value;
	cmd->mask = CRYPTO_MASK_ALL;
}

/*
 * nss_crypto_read_cblk()
 * 	read CMD block data
 */
static inline uint32_t nss_crypto_read_cblk(struct nss_crypto_bam_cmd *cmd)
{
	return (cmd->value & cmd->mask);
}
/*
 * nss_crypto_setup_cmd_config()
 * 	setup the common command block, 1 per session
 */
static void nss_crypto_setup_cmd_config(struct nss_crypto_cmd_config *cfg, uint32_t base_addr, uint16_t pp_num,
					struct nss_crypto_encr_cfg *encr, struct nss_crypto_auth_cfg *auth)
{
	uint32_t *key_ptr, key_val;
	uint32_t cfg_value, beat;
	int i;

	/*
	 * Configuration programming
	 * - beats
	 * - interrupts
	 * - pipe number for the crypto transaction
	 */
	beat = CRYPTO_BURST2BEATS(CRYPTO_MAX_BURST);

	cfg_value = 0;
	cfg_value |= CRYPTO_CONFIG_DOP_INTR; /* operation interrupt */
	cfg_value |= CRYPTO_CONFIG_DIN_INTR; /* input interrupt */
	cfg_value |= CRYPTO_CONFIG_DOUT_INTR; /* output interrupt */
	cfg_value |= CRYPTO_CONFIG_PIPE_SEL(pp_num); /* pipe pair number to use */
	cfg_value |= CRYPTO_CONFIG_REQ_SIZE(beat); /* BAM DMA maximum beat size */

	nss_crypto_write_cblk(&cfg->config_0, CRYPTO_CONFIG + base_addr, cfg_value);
	nss_crypto_write_cblk(&cfg->encr_seg_cfg, CRYPTO_ENCR_SEG_CFG + base_addr, encr->cfg);
	nss_crypto_write_cblk(&cfg->auth_seg_cfg, CRYPTO_AUTH_SEG_CFG + base_addr, auth->cfg);

	nss_crypto_update_cblk_mask(&cfg->encr_seg_cfg, !encr->cfg);
	nss_crypto_update_cblk_mask(&cfg->auth_seg_cfg, !auth->cfg);

	nss_crypto_write_cblk(&cfg->encr_ctr_msk, CRYPTO_ENCR_CNTR_MASK + base_addr, 0xffffffff);

	/*
	 * auth IV update
	 */
	for (i = 0; i < NSS_CRYPTO_AUTH_IV_REGS; i++) {
		nss_crypto_write_cblk(&cfg->auth_iv[i], CRYPTO_AUTH_IVn(i) + base_addr, auth->iv[i]);
		nss_crypto_update_cblk_mask(&cfg->auth_iv[i], !auth->cfg);
	}

	/*
	 * cipher key update
	 */
	for (i = 0; i < NSS_CRYPTO_CKEY_REGS; i++) {
		key_ptr = (uint32_t *)&encr->key[i * 4];
		key_val = cpu_to_be32(*key_ptr);

		nss_crypto_write_cblk(&cfg->keys.encr[i], CRYPTO_ENCR_KEYn(i) + base_addr, key_val);
	}

	/*
	 * auth key update
	 */
	for (i = 0; i < NSS_CRYPTO_AKEY_REGS; i++) {
		key_ptr = (uint32_t *)&auth->key[i * 4];
		key_val = cpu_to_be32(*key_ptr);

		nss_crypto_write_cblk(&cfg->keys.auth[i], CRYPTO_AUTH_KEYn(i) + base_addr, key_val);
		nss_crypto_update_cblk_mask(&cfg->keys.auth[i], !auth->cfg);
	}

}

/*
 * nss_crypto_setup_cmd_request()
 * 	setup the per request command block
 */
static void nss_crypto_setup_cmd_request(struct nss_crypto_cmd_request *req, uint32_t base_addr, uint16_t pp_num)
{
	uint32_t cfg_value;
	uint32_t beat;
	int i;

	nss_crypto_write_cblk(&req->seg_size, CRYPTO_SEG_SIZE + base_addr, 0);
	nss_crypto_write_cblk(&req->encr_seg_size, CRYPTO_ENCR_SEG_SIZE + base_addr, 0);
	nss_crypto_write_cblk(&req->auth_seg_size, CRYPTO_AUTH_SEG_SIZE + base_addr, 0);

	/*
	 * cipher IV reset
	 */
	for (i = 0; i < NSS_CRYPTO_CIPHER_IV_REGS; i++) {
		nss_crypto_write_cblk(&req->encr_iv[i], CRYPTO_ENCR_IVn(i) + base_addr, 0);
	}

	/*
	 * Configuration programming
	 * - beats
	 * - interrupts
	 * - pipe number for the crypto transaction
	 */
	beat = CRYPTO_BURST2BEATS(CRYPTO_MAX_BURST);

	cfg_value = 0;
	cfg_value |= CRYPTO_CONFIG_DOP_INTR; /* operation interrupt */
	cfg_value |= CRYPTO_CONFIG_DIN_INTR; /* input interrupt */
	cfg_value |= CRYPTO_CONFIG_DOUT_INTR; /* output interrupt */
	cfg_value |= CRYPTO_CONFIG_PIPE_SEL(pp_num); /* pipe pair number to use */
	cfg_value |= CRYPTO_CONFIG_REQ_SIZE(beat); /* BAM DMA maximum beat size */
	cfg_value |= CRYPTO_CONFIG_LITTLE_ENDIAN; /* switch to little endian mode */

	nss_crypto_write_cblk(&req->config_1, CRYPTO_CONFIG + base_addr, cfg_value);

	/*
	 * go_proc or crypto trigger programming
	 */
	cfg_value = 0;
	cfg_value |= CRYPTO_GOPROC_SET; /* go proc */
	cfg_value |= CRYPTO_GOPROC_RESULTS_DUMP; /* generate results dump */

	nss_crypto_write_cblk(&req->go_proc, CRYPTO_GO_PROC + base_addr, cfg_value);

	/*
	 * unlock cmd using dummy debug register enable
	 */
	nss_crypto_write_cblk(&req->unlock, CRYPTO_DEBUG_ENABLE + base_addr, 0x1);
}

/*
 * nss_crypto_bam_init()
 * 	initialize  the BAM pipe; pull it out reset and load its configuration
 */
static int nss_crypto_bam_pipe_init(struct nss_crypto_ctrl_eng *ctrl, uint32_t pipe)
{
	uint32_t cfg;
	uint32_t in_pipe;

	/*
	 * Put and Pull BAM pipe from reset
	 */
	iowrite32(0x1, ctrl->bam_base + CRYPTO_BAM_P_RST(pipe));
	iowrite32(0x0, ctrl->bam_base + CRYPTO_BAM_P_RST(pipe));

	in_pipe = NSS_CRYPTO_INPIPE(pipe);

	/*
	 * set the following
	 * - direction IN or OUT
	 * - BAM mode is system
	 * - enable the pipe
	 * - BAM pipe lock group common for IN & OUT
	 */
	cfg = 0;
	cfg |= CRYPTO_BAM_P_CTRL_DIRECTION(pipe);
	cfg |= CRYPTO_BAM_P_CTRL_SYS_MODE;
	cfg |= CRYPTO_BAM_P_CTRL_EN;
	cfg |= CRYPTO_BAM_P_CTRL_LOCK_GROUP(in_pipe);

	iowrite32(cfg, ctrl->bam_base + CRYPTO_BAM_P_CTRL(pipe));

	nss_crypto_dbg("BAM_CTRL = 0x%x, pipe = %d\n", ioread32(ctrl->bam_base + CRYPTO_BAM_P_CTRL(pipe)), pipe);

	return 0;
}

/*
 * nss_crypto_desc_alloc()
 * 	allocate crypto descriptor memory from DDR
 *
 * this allocates coherent memory for crypto descriptors, the pipe initialization should
 * tell the size
 */
static void *nss_crypto_desc_alloc(struct device *dev, uint32_t *paddr, uint32_t size)
{
	void *ret_addr;
	uint32_t phy_addr;

	size = ALIGN(size, NSS_CRYPTO_DESC_ALIGN);

	ret_addr = kzalloc(size, GFP_KERNEL | GFP_DMA);
	if (!ret_addr) {
		nss_crypto_err("OOM: unable to allocate memory of size(%dKB)\n", NSS_CRYPTO_SIZE_KB(size));
		return NULL;
	}

	BUG_ON(((uint32_t)ret_addr % NSS_CRYPTO_DESC_ALIGN));

	phy_addr = dma_map_single(dev, ret_addr, size, DMA_TO_DEVICE);
	if (!phy_addr) {
		nss_crypto_err("%p:unable to map Vaddr(%p)\n", dev, ret_addr);
		goto fail;
	}

	*paddr = phy_addr;
	return ret_addr;

fail:
	kfree(ret_addr);
	return NULL;
}

/*
 * nss_crypto_pipe_init()
 * 	initialize the crypto pipe
 *
 * this will
 * - configure the BAM pipes
 * - allocate the descriptors
 * - program the BAM descriptors with the command blocks (lock/unlock)
 * - update the BAM registers for the ring locations
 */
void nss_crypto_pipe_init(struct nss_crypto_ctrl_eng *eng, uint32_t idx, uint32_t *desc_paddr, struct nss_crypto_desc **desc_vaddr)
{
	struct nss_crypto_desc *desc;
	uint32_t in_pipe, out_pipe;
	uint32_t in_pipe_sz, out_pipe_sz;
	uint32_t paddr;
	int i;

	/*
	 * Init the Crypto Core
	 */
	in_pipe = nss_crypto_idx_to_inpipe(idx);
	out_pipe = nss_crypto_idx_to_outpipe(idx);

	in_pipe_sz = sizeof(struct nss_crypto_in_trans) * NSS_CRYPTO_MAX_QDEPTH;
	out_pipe_sz = sizeof(struct nss_crypto_out_trans) * NSS_CRYPTO_MAX_QDEPTH;

	nss_crypto_bam_pipe_init(eng, in_pipe);
	nss_crypto_bam_pipe_init(eng, out_pipe);

	/*
	 * Allocate descriptors
	 */
	desc = nss_crypto_desc_alloc(eng->dev, &paddr, NSS_CRYPTO_DESC_SZ);
	if (!desc) {
		nss_crypto_err("%p:unable to allocate BAM pipe descriptors\n", eng);
		return;
	}

	*desc_paddr = paddr;
	*desc_vaddr = desc;

	/*
	 * write input BAM ring
	 */
	iowrite32(paddr, eng->bam_base + CRYPTO_BAM_P_DESC_FIFO_ADDR(in_pipe));
	iowrite32(in_pipe_sz, eng->bam_base + CRYPTO_BAM_P_FIFO_SIZES(in_pipe));

	/*
	 * write output BAM ring
	 */
	iowrite32(paddr + in_pipe_sz, eng->bam_base + CRYPTO_BAM_P_DESC_FIFO_ADDR(out_pipe));
	iowrite32(out_pipe_sz, eng->bam_base + CRYPTO_BAM_P_FIFO_SIZES(out_pipe));

	/*
	 * this loop pre-fills the pipe rings with the command blocks, the data path will
	 * no longer need to write the command block locations when sending the packets for
	 * encryption/decryption. The idea header is to avoid as much as possible the writes
	 * to the uncached locations.
	 */
	for (i = 0; i < NSS_CRYPTO_MAX_QDEPTH; i++) {
		/*
		 * program CMD0 (encr configs & auth configs)
		 */
		desc->in[i].cmd0_lock.data_len = 0;
		desc->in[i].cmd0_lock.data_start = 0x0;
		desc->in[i].cmd0_lock.flags = (CRYPTO_BAM_DESC_CMD | CRYPTO_BAM_DESC_LOCK);

		/*
		 * program CMD1 (config & go_proc)
		 */
		desc->in[i].cmd1.data_len = NSS_CRYPTO_CMD_REQ_SZ;
		desc->in[i].cmd1.data_start = 0x0;
		desc->in[i].cmd1.flags = CRYPTO_BAM_DESC_CMD;

		desc->in[i].data.flags = (CRYPTO_BAM_DESC_EOT | CRYPTO_BAM_DESC_NWD);

		/*
		 * program CMD2 (unlock)
		 */
		desc->in[i].cmd2_unlock.data_len = NSS_CRYPTO_CMD_UNLOCK_SZ;
		desc->in[i].cmd2_unlock.data_start = 0x0;
		desc->in[i].cmd2_unlock.flags = (CRYPTO_BAM_DESC_CMD | CRYPTO_BAM_DESC_UNLOCK);

		desc->out[i].data.flags = 0;

		/*
		 * program results dump
		 */
		desc->out[i].results.data_len = NSS_CRYPTO_RESULTS_SZ;
	}

	nss_crypto_info_always("init completed for Pipe Pair[%d]\n", idx);
	nss_crypto_dbg("total size - %d, qdepth - %d, in_sz - %d, out_sz - %d\n",
			NSS_CRYPTO_DESC_SZ, NSS_CRYPTO_MAX_QDEPTH,
			in_pipe_sz, out_pipe_sz);

}

/*
 * nss_crypto_program_ckeys()
 * 	this will program cipher key registers with new keys
 */
static void nss_crypto_write_ckey_regs(uint8_t *base, uint16_t pp_num, struct nss_crypto_encr_cfg *encr)
{
	uint32_t key_val, *key_ptr;
	int i;

	nss_crypto_info("updating cached cipher keys\n");

	for (i = 0; i < NSS_CRYPTO_CKEY_REGS ; i++) {
		key_ptr = (uint32_t *)&encr->key[i * 4];
		key_val = cpu_to_be32(*key_ptr);

		if (!key_val) {
			break;
		}

		iowrite32(key_val, CRYPTO_ENCR_PIPEm_KEYn(pp_num, i) + base);
	}
}

/*
 * nss_crypto_program_akeys()
 * 	this will program authentication key registers with new keys
 */
static void nss_crypto_write_akey_regs(uint8_t *base, uint16_t pp_num, struct nss_crypto_auth_cfg *auth)
{
	uint32_t key_val, *key_ptr;
	int i;

	nss_crypto_info("updating cached auth keys\n");

	for (i = 0; i < NSS_CRYPTO_AKEY_REGS; i++) {
		key_ptr = (uint32_t *)&auth->key[i * 4];
		key_val = cpu_to_be32(*key_ptr);

		if (!key_val) {
			break;
		}

		iowrite32(key_val, CRYPTO_AUTH_PIPEm_KEYn(pp_num, i) + base);
	}
}

/*
 * nss_crypto_validate_cipher()
 * 	for a given cipher check whether the programming done by CFI is valid
 *
 * this is supposed to verify the
 * - that the algorithm is supported
 * - that key size is supported
 */
static nss_crypto_status_t nss_crypto_validate_cipher(struct nss_crypto_key *cipher, struct nss_crypto_encr_cfg *encr_cfg)
{
	encr_cfg->cfg = 0;

	/*
	 * NONE, always the first check
	 */
	if (cipher == NULL) {
		memcpy(encr_cfg->key, null_ckey, NSS_CRYPTO_CKEY_SZ);

		return NSS_CRYPTO_STATUS_OK;
	}

	nss_crypto_err("validating cipher (algo = %d, key_len = %d)\n", cipher->algo, cipher->key_len);

	/*
	 * AES-128
	 */
	if ((cipher->algo == NSS_CRYPTO_CIPHER_AES) && (cipher->key_len == NSS_CRYPTO_KEYLEN_AES128)) {
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_KEY_AES128;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_ALG_AES;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_MODE_CBC;

		memcpy(encr_cfg->key, cipher->key, NSS_CRYPTO_KEYLEN_AES128);

		return NSS_CRYPTO_STATUS_OK;
	}

	/*
	 * AES-256
	 */
	if ((cipher->algo == NSS_CRYPTO_CIPHER_AES) && (cipher->key_len == NSS_CRYPTO_KEYLEN_AES256)) {
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_KEY_AES256;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_ALG_AES;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_MODE_CBC;

		memcpy(encr_cfg->key, cipher->key, NSS_CRYPTO_KEYLEN_AES256);

		return NSS_CRYPTO_STATUS_OK;
	}

	/*
	 * DES-64 (SINGLE_DES)
	 */
	if ((cipher->algo == NSS_CRYPTO_CIPHER_DES) && (cipher->key_len == NSS_CRYPTO_KEYLEN_DES)) {
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_KEY_SINGLE_DES;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_ALG_DES;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_MODE_CBC;

		memcpy(encr_cfg->key, cipher->key, NSS_CRYPTO_KEYLEN_DES);

		return NSS_CRYPTO_STATUS_OK;
	}

	/*
	 * DES-192 (TRIPLE_DES)
	 */
	if ((cipher->algo == NSS_CRYPTO_CIPHER_DES) && (cipher->key_len == NSS_CRYPTO_KEYLEN_3DES)) {
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_KEY_TRIPLE_DES;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_ALG_DES;
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_MODE_CBC;

		memcpy(encr_cfg->key, cipher->key, NSS_CRYPTO_KEYLEN_3DES);

		return NSS_CRYPTO_STATUS_OK;
	}

	return NSS_CRYPTO_STATUS_ENOSUPP;
}

/*
 * nss_crypto_validate_auth()
 * 	for a given auth validate the programming done by CFI
 *
 * this is supposed to verify the
 * - that the algorithm is supported
 * - that key size is supported
 */
static nss_crypto_status_t nss_crypto_validate_auth(struct nss_crypto_key *auth, struct nss_crypto_auth_cfg *auth_cfg)
{
	auth_cfg->iv = NULL;
	auth_cfg->cfg = 0;

	/*
	 * NONE, always the first check
	 */
	if (auth == NULL) {
		auth_cfg->iv = (uint32_t *)&null_iv[0];

		memcpy(auth_cfg->key, null_akey, NSS_CRYPTO_AKEY_SZ);

		return NSS_CRYPTO_STATUS_OK;
	}

	nss_crypto_info("validating auth (algo = %d, key_len = %d)\n", auth->algo, auth->key_len);

	/*
	 * SHA1-HMAC
	 */
	if ((auth->algo == NSS_CRYPTO_AUTH_SHA1_HMAC) && (auth->key_len == NSS_CRYPTO_KEYLEN_SHA1HMAC)) {
		auth_cfg->iv = (uint32_t *)&fips_sha1_iv[0];

		auth_cfg->cfg |= CRYPTO_AUTH_SEG_CFG_MODE_HMAC;
		auth_cfg->cfg |= (CRYPTO_AUTH_SEG_CFG_ALG_SHA | CRYPTO_AUTH_SEG_CFG_SIZE_SHA1);
		auth_cfg->cfg |= (CRYPTO_AUTH_SEG_CFG_FIRST | CRYPTO_AUTH_SEG_CFG_LAST);

		memcpy(auth_cfg->key, auth->key, NSS_CRYPTO_KEYLEN_SHA1HMAC);

		return NSS_CRYPTO_STATUS_OK;
	}

	/*
	 * SHA256-HMAC
	 */
	if ((auth->algo == NSS_CRYPTO_AUTH_SHA256_HMAC) && (auth->key_len == NSS_CRYPTO_KEYLEN_SHA256HMAC)) {

		auth_cfg->iv = (uint32_t *)&fips_sha256_iv[0];

		auth_cfg->cfg |= CRYPTO_AUTH_SEG_CFG_MODE_HMAC;
		auth_cfg->cfg |= (CRYPTO_AUTH_SEG_CFG_ALG_SHA | CRYPTO_AUTH_SEG_CFG_SIZE_SHA2);
		auth_cfg->cfg |= (CRYPTO_AUTH_SEG_CFG_FIRST | CRYPTO_AUTH_SEG_CFG_LAST);

		memcpy(auth_cfg->key, auth->key, NSS_CRYPTO_KEYLEN_SHA256HMAC);

		return NSS_CRYPTO_STATUS_OK;
	}

	return NSS_CRYPTO_STATUS_ENOSUPP;
}

/*
 * nss_crypto_key_update()
 * 	update the newly arrived keys/algorithm from session alloc
 *
 * this will do the following
 * - pre-fill the command blocks with cipher/auth specific data
 * - write new keys to the cipher/auth registers
 *
 */
static void nss_crypto_key_update(struct nss_crypto_ctrl_eng *eng, uint32_t idx, struct nss_crypto_encr_cfg *encr_cfg,
					struct nss_crypto_auth_cfg *auth_cfg)
{
	const uint32_t size = sizeof(struct nss_crypto_cmd_block);
	struct nss_crypto_ctrl_idx *ctrl_idx;
	struct nss_crypto_cmd_block *cblk;
	uint16_t pp_num;
	uint32_t paddr;
	int i = 0;

	ctrl_idx = &eng->idx_tbl[idx];
	pp_num = ctrl_idx->idx.pp_num;
	cblk = ctrl_idx->cblk;

	/*
	 * if the indexes are within cached range and force uncached is not set,
	 * then program the registers
	 */
	if ((idx < NSS_CRYPTO_MAX_CACHED_IDXS) && !CONFIG_NSS_CRYPTO_FORCE_UNCACHE) {
		encr_cfg->cfg |= CRYPTO_ENCR_SEG_CFG_PIPE_KEYS;
		auth_cfg->cfg |= CRYPTO_AUTH_SEG_CFG_PIPE_KEYS;

		nss_crypto_write_ckey_regs(eng->crypto_base, pp_num, encr_cfg);
		nss_crypto_write_akey_regs(eng->crypto_base, pp_num, auth_cfg);
	}

	paddr = ctrl_idx->idx.cblk_paddr;

	dma_unmap_single(eng->dev, paddr, size, DMA_FROM_DEVICE);

	/*
	 * configuration command setup, this is 1 per session
	 */
	nss_crypto_setup_cmd_config(&cblk->cfg, eng->cmd_base, pp_num, encr_cfg, auth_cfg);

	/*
	 * request command setup, these are 'n' per session; where n = MAX_QDEPTH
	 */
	for (i = 0; i < NSS_CRYPTO_MAX_QDEPTH; i++) {
		nss_crypto_setup_cmd_request(&cblk->req[i], eng->cmd_base, pp_num);
	}

	paddr = dma_map_single(eng->dev, ctrl_idx->cblk, size, DMA_TO_DEVICE);

	BUG_ON(paddr != ctrl_idx->idx.cblk_paddr);
}

/*
 * nss_crypto_cblk_update()
 * 	update the configuration command block using buffer informantion
 */
static inline nss_crypto_status_t nss_crypto_cblk_update(struct nss_crypto_ctrl_eng *eng, struct nss_crypto_cmd_block *cblk,
						struct nss_crypto_params *params)
{
	struct nss_crypto_cmd_config *cfg;
	uint32_t base_addr = eng->cmd_base;
	uint32_t encr_cfg = 0, auth_cfg = 0;
	const uint16_t req_mask = (NSS_CRYPTO_REQ_TYPE_DECRYPT | NSS_CRYPTO_REQ_TYPE_ENCRYPT);

	cfg = &cblk->cfg;

	/*
	 * update the skip values as the assumption is that it remains constant for a session
	 */
	nss_crypto_write_cblk(&cfg->encr_seg_start, CRYPTO_ENCR_SEG_START + base_addr, params->cipher_skip);
	nss_crypto_write_cblk(&cfg->auth_seg_start, CRYPTO_AUTH_SEG_START + base_addr, params->auth_skip);

	/*
	 * update the segment configuration for encrypt or decrypt type
	 */
	encr_cfg = nss_crypto_read_cblk(&cfg->encr_seg_cfg);
	auth_cfg = nss_crypto_read_cblk(&cfg->auth_seg_cfg);

	switch (params->req_type & req_mask) {
	case NSS_CRYPTO_REQ_TYPE_ENCRYPT:
		encr_cfg |= CRYPTO_ENCR_SEG_CFG_ENC;
		auth_cfg |= CRYPTO_AUTH_SEG_CFG_POS_AFTER;
		break;

	case NSS_CRYPTO_REQ_TYPE_DECRYPT:
		encr_cfg &= ~CRYPTO_ENCR_SEG_CFG_ENC;
		auth_cfg |= CRYPTO_AUTH_SEG_CFG_POS_BEFORE;
		break;

	default:
		nss_crypto_err("unknown request type\n");
		return NSS_CRYPTO_STATUS_EINVAL;
	}

	nss_crypto_write_cblk(&cfg->encr_seg_cfg, CRYPTO_ENCR_SEG_CFG + base_addr, encr_cfg);
	nss_crypto_write_cblk(&cfg->auth_seg_cfg, CRYPTO_AUTH_SEG_CFG + base_addr, auth_cfg);

	return NSS_CRYPTO_STATUS_OK;
}

/*
 * nss_crypto_update_cipher_info()
 * 	update the cipher info into the index info table
 */
void nss_crypto_update_cipher_info(struct nss_crypto_idx_info *idx, struct nss_crypto_key *cipher)
{
	idx->ckey.algo = cipher ? cipher->algo : NSS_CRYPTO_CIPHER_NONE;
	idx->ckey.key_len = cipher ? cipher->key_len : 0;
	idx->ckey.key = NULL;
}

/*
 * nss_crypto_update_idx_state()
 *	Updates the session state
 */
void nss_crypto_update_idx_state(struct nss_crypto_idx_info *idx, enum nss_crypto_session_state state)
{
	idx->state = state;
}

/*
 * nss_crypto_update_idx_reqtype()
 *	update the session crypto request type (encryption/decryption)
 */
static nss_crypto_status_t nss_crypto_update_idx_reqtype(struct nss_crypto_idx_info *idx, uint16_t req_type)
{
	const uint16_t req_mask = (NSS_CRYPTO_REQ_TYPE_DECRYPT | NSS_CRYPTO_REQ_TYPE_ENCRYPT);

	/*
	 * check if the call is for resetting the state
	 */
	if (req_type == NSS_CRYPTO_REQ_TYPE_NONE) {
		idx->req_type = req_type;
		return NSS_CRYPTO_STATUS_OK;
	}

	switch (req_type & req_mask) {
	case NSS_CRYPTO_REQ_TYPE_ENCRYPT:
	case NSS_CRYPTO_REQ_TYPE_DECRYPT:
		idx->req_type = req_type;
		break;

	default:
		nss_crypto_err("unknown request type 0x%x\n", req_type);
		return NSS_CRYPTO_STATUS_EINVAL;
	}
	return NSS_CRYPTO_STATUS_OK;
}

/*
 * nss_crypto_update_auth_info()
 * 	update the auth info into the index info table
 */
void nss_crypto_update_auth_info(struct nss_crypto_idx_info *idx, struct nss_crypto_key *auth)
{
	idx->akey.algo = auth ? auth->algo : NSS_CRYPTO_AUTH_NONE;
	idx->akey.key_len = auth ? auth->key_len : 0;
	idx->akey.key = NULL;
}

/*
 * nss_crypto_chk_idx_isfree()
 *	get the session state
 */
bool nss_crypto_chk_idx_isfree(struct nss_crypto_idx_info *idx)
{
	bool session_state = false;

	if (idx->state != NSS_CRYPTO_SESSION_STATE_ACTIVE) {
		session_state = true;
	}

	return session_state;
}

/*
 * nss_crypto_session_update()
 * 	update allocated crypto session parameters
 */
nss_crypto_status_t nss_crypto_session_update(nss_crypto_handle_t crypto, uint32_t session_idx, struct nss_crypto_params *params)
{
	const uint32_t size = sizeof(struct nss_crypto_cmd_block);
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_ctrl_eng *e_ctrl = &ctrl->eng[0];
	nss_crypto_status_t status = NSS_CRYPTO_STATUS_OK;
	struct nss_crypto_ctrl_idx *ctrl_idx;
	uint32_t idx_mask;
	uint32_t paddr;
	int i = 0;

	idx_mask = (0x1 << session_idx);

	/*
	 * check if session is a valid active session
	 */
	if ((ctrl->idx_bitmap & idx_mask) != idx_mask) {
		nss_crypto_err("invalid session index\n");
		return NSS_CRYPTO_STATUS_FAIL;
	}

	/*
	 * Check if the common command block parameters are already set for
	 * this session
	 */
	if (nss_crypto_check_idx_state(ctrl->idx_state_bitmap, session_idx)) {
		nss_crypto_dbg("Duplicate session update request\n");
		return NSS_CRYPTO_STATUS_OK;
	}

	rmb();
	nss_crypto_set_idx_state(&ctrl->idx_state_bitmap, session_idx);

	/*
	 * Update whether this SA index is used for encryption or decryption
	 * TODO: Need to evaluate this for the case of pure authentication
	 */
	status = nss_crypto_update_idx_reqtype(&ctrl->idx_info[session_idx], params->req_type);
	if (status != NSS_CRYPTO_STATUS_OK) {
		nss_crypto_err("invalid parameters\n");
		return NSS_CRYPTO_STATUS_EINVAL;
	}

	for (i = 0; i < ctrl->num_eng; i++, e_ctrl++) {
		ctrl_idx = &e_ctrl->idx_tbl[session_idx];
		paddr = ctrl_idx->idx.cblk_paddr;

		/*
		 * Update session specific config data
		 */
		dma_unmap_single(e_ctrl->dev, paddr, size, DMA_FROM_DEVICE);

		status = nss_crypto_cblk_update(e_ctrl, ctrl_idx->cblk, params);
		if (status != NSS_CRYPTO_STATUS_OK) {
			nss_crypto_err("invalid parameters\n");
			return NSS_CRYPTO_STATUS_EINVAL;
		}

		paddr = dma_map_single(e_ctrl->dev, ctrl_idx->cblk, size, DMA_TO_DEVICE);

		BUG_ON(paddr != ctrl_idx->idx.cblk_paddr);
	}
	return NSS_CRYPTO_STATUS_OK;
}
EXPORT_SYMBOL(nss_crypto_session_update);

/*
 * nss_crypto_session_alloc()
 * 	allocate a new crypto session for operation
 */
nss_crypto_status_t nss_crypto_session_alloc(nss_crypto_handle_t crypto, struct nss_crypto_key *cipher, struct nss_crypto_key *auth,
						uint32_t *session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_encr_cfg encr_cfg;
	struct nss_crypto_auth_cfg auth_cfg;
	enum nss_crypto_cipher cipher_algo;
	nss_crypto_status_t status;
	bool first_idx;
	uint32_t idx;
	int i;

	memset(&encr_cfg, 0, sizeof(struct nss_crypto_encr_cfg));
	memset(&auth_cfg, 0, sizeof(struct nss_crypto_auth_cfg));

	spin_lock_bh(&ctrl->lock); /* index lock*/

	if (ctrl->num_idxs == NSS_CRYPTO_MAX_IDXS) {
		spin_unlock_bh(&ctrl->lock); /* index unlock*/
		nss_crypto_err("index table full\n");
		return NSS_CRYPTO_STATUS_ENOMEM;
	}

	nss_crypto_assert(ctrl->num_idxs <= NSS_CRYPTO_MAX_IDXS);

	/*
	 * validate cipher
	 */
	status = nss_crypto_validate_cipher(cipher, &encr_cfg);
	if (status != NSS_CRYPTO_STATUS_OK) {
		spin_unlock_bh(&ctrl->lock); /* index unlock*/
		nss_crypto_err("invalid cipher configuration\n");
		return status;
	}

	/*
	 * validate authentication
	 */
	status = nss_crypto_validate_auth(auth, &auth_cfg);
	if (status != NSS_CRYPTO_STATUS_OK) {
		spin_unlock_bh(&ctrl->lock); /* index unlock*/
		nss_crypto_err("invalid auth configuration\n");
		return status;
	}

	/*
	 * is this the first session that we are creating
	 */
	first_idx = !ctrl->idx_bitmap;

	/*
	 * search a free index and allocate it
	 */
	idx = ffz(ctrl->idx_bitmap);

	ctrl->num_idxs++;
	ctrl->idx_bitmap |= (0x1 << idx);
	nss_crypto_clear_idx_state(&ctrl->idx_state_bitmap, idx);

	nss_crypto_update_cipher_info(&ctrl->idx_info[idx], cipher);
	nss_crypto_update_auth_info(&ctrl->idx_info[idx], auth);
	nss_crypto_update_idx_state(&ctrl->idx_info[idx], NSS_CRYPTO_SESSION_STATE_ACTIVE);

	/*
	 * program keys for all the engines for the given pipe pair (index)
	 */
	for (i = 0; i < ctrl->num_eng; i++) {
		nss_crypto_key_update(&ctrl->eng[i], idx, &encr_cfg, &auth_cfg);
	}

	*session_idx = idx;

	cipher_algo = cipher ? cipher->algo : NSS_CRYPTO_CIPHER_NULL;
	nss_crypto_send_session_update(idx, NSS_CRYPTO_SESSION_STATE_ACTIVE, cipher_algo);

	spin_unlock_bh(&ctrl->lock); /* index unlock*/

	/*
	 * scale the fabric up to turbo as this the first index
	 */
	if (unlikely(first_idx)) {
		nss_pm_set_perf_level(gbl_ctx.pm_hdl, NSS_PM_PERF_LEVEL_TURBO);
	}

	nss_crypto_info_always("new index (used - %d, max - %d)\n", ctrl->num_idxs, NSS_CRYPTO_MAX_IDXS);
	nss_crypto_dbg("index bitmap = 0x%x, index assigned = %d\n", ctrl->idx_bitmap, idx);

	return NSS_CRYPTO_STATUS_OK;
}
EXPORT_SYMBOL(nss_crypto_session_alloc);

/*
 * nss_crypto_session_free()
 * 	free the crypto session, that was previously allocated
 */
nss_crypto_status_t nss_crypto_session_free(nss_crypto_handle_t crypto, uint32_t session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	uint32_t idx_mask;

	idx_mask = (0x1 << session_idx);

	spin_lock_bh(&ctrl->lock); /* index lock */

	if (!ctrl->num_idxs || ((ctrl->idx_bitmap & idx_mask) != idx_mask)) {
		spin_unlock_bh(&ctrl->lock);
		nss_crypto_err("crypto index(%d) is invalid\n", session_idx);
		return NSS_CRYPTO_STATUS_EINVAL;
	}

	spin_unlock_bh(&ctrl->lock); /* index unlock*/

	/*
	 * The reset session is sent to NSS. The response will trigger the
	 * timer for delayed freeing of the session. After the timeout
	 * resources attached to session index will be deallocated
	 */
	nss_crypto_send_session_update(session_idx, NSS_CRYPTO_SESSION_STATE_FREE, NSS_CRYPTO_CIPHER_NULL);

	return NSS_CRYPTO_STATUS_OK;
}
EXPORT_SYMBOL(nss_crypto_session_free);

/*
 * nss_crypto_idx_free()
 * 	De-allocate all associated resources with session
 */
void nss_crypto_idx_free(unsigned long session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_encr_cfg encr_cfg;
	struct nss_crypto_auth_cfg auth_cfg;
	uint32_t idx_mask;
	bool last_idx;
	int i;

	idx_mask = (0x1 << session_idx);

	memset(&encr_cfg, 0, sizeof(struct nss_crypto_encr_cfg));
	memset(&auth_cfg, 0, sizeof(struct nss_crypto_auth_cfg));

	spin_lock(&ctrl->lock); /* index lock*/

	/*
	 * NULL configuration
	 */
	encr_cfg.cfg = 0;
	auth_cfg.cfg = 0;
	auth_cfg.iv = (uint32_t *)&null_iv[0];

	memcpy(encr_cfg.key, null_ckey, NSS_CRYPTO_CKEY_SZ);
	memcpy(auth_cfg.key, null_akey, NSS_CRYPTO_AKEY_SZ);

	nss_crypto_update_cipher_info(&ctrl->idx_info[session_idx], NULL);
	nss_crypto_update_auth_info(&ctrl->idx_info[session_idx], NULL);
	nss_crypto_update_idx_state(&ctrl->idx_info[session_idx], NSS_CRYPTO_SESSION_STATE_FREE);
	nss_crypto_update_idx_reqtype(&ctrl->idx_info[session_idx], NSS_CRYPTO_REQ_TYPE_NONE);

	/*
	 * program keys for all the engines for the given pipe pair (index)
	 */
	for (i = 0; i < ctrl->num_eng; i++) {
		nss_crypto_key_update(&ctrl->eng[i], session_idx, &encr_cfg, &auth_cfg);
	}

	ctrl->idx_bitmap &= ~idx_mask;
	ctrl->num_idxs--;

	/*
	 * check if this the last index that is getting deleted
	 */
	last_idx = !ctrl->idx_bitmap;

	spin_unlock(&ctrl->lock); /* index unlock*/

	/*
	 * scale the fabric down to IDLE as this the last index
	 */
	if (unlikely(last_idx)) {
		nss_pm_set_perf_level(gbl_ctx.pm_hdl, NSS_PM_PERF_LEVEL_IDLE);
	}

	nss_crypto_info_always("deallocated index (used - %d, max - %d)\n", ctrl->num_idxs, NSS_CRYPTO_MAX_IDXS);
	nss_crypto_dbg("index freed  = 0x%x, index = %d\n", ctrl->idx_bitmap, session_idx);
}

/*
 * nss_crypto_get_cipher()
 * 	return the cipher algo with the associated session
 */
enum nss_crypto_cipher nss_crypto_get_cipher(uint32_t session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_idx_info *idx;

	idx = &ctrl->idx_info[session_idx];

	return idx->ckey.algo;

}
EXPORT_SYMBOL(nss_crypto_get_cipher);

/*
 * nss_crypto_get_cipher_keylen()
 * 	return the cipher key length with the associated session
 */
uint32_t nss_crypto_get_cipher_keylen(uint32_t session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_idx_info *idx;

	idx = &ctrl->idx_info[session_idx];

	return idx->ckey.key_len;
}
EXPORT_SYMBOL(nss_crypto_get_cipher_keylen);

/*
 * nss_crypto_get_reqtype()
 * 	return the transform type of the associated session
 */
uint32_t nss_crypto_get_reqtype(uint32_t session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_idx_info *idx;

	idx = &ctrl->idx_info[session_idx];

	return idx->req_type;
}
EXPORT_SYMBOL(nss_crypto_get_reqtype);

/*
 * nss_crypto_get_auth()
 * 	return the auth algo with the associated session
 */
enum nss_crypto_auth nss_crypto_get_auth(uint32_t session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_idx_info *idx;

	idx = &ctrl->idx_info[session_idx];

	return idx->akey.algo;

}
EXPORT_SYMBOL(nss_crypto_get_auth);

/*
 * nss_crypto_get_auth_keylen()
 * 	return the auth key length with the associated session
 */
uint32_t nss_crypto_get_auth_keylen(uint32_t session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_idx_info *idx;

	idx = &ctrl->idx_info[session_idx];

	return idx->akey.key_len;

}
EXPORT_SYMBOL(nss_crypto_get_auth_keylen);

/*
 * nss_crypto_start_idx_free()
 *  	Start the timer for session deallocation request
 */
bool nss_crypto_start_idx_free(uint32_t session_idx)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_idx_info *idx;

	idx = &ctrl->idx_info[session_idx];

	/*
	 * Scedule the timer if it is not already active
	 */
	if (timer_pending(&idx->free_timer)) {
		nss_crypto_err("timer is already pending\n");

		return false;
	}

	idx->free_timer.expires = jiffies + NSS_CRYPTO_SESSION_FREE_DELAY_TICKS;
	idx->free_timer.data = session_idx;
	idx->free_timer.function = nss_crypto_idx_free;

	mod_timer(&idx->free_timer, idx->free_timer.expires);

	return true;
}

/*
 * nss_crypto_idx_init()
 * 	initialize the index table
 *
 * note: this also allocates the command blocks and copies the allocated indexes
 * into a message so that it can be sent to NSS
 */
nss_crypto_status_t nss_crypto_idx_init(struct nss_crypto_ctrl_eng *eng, struct nss_crypto_idx msg[])
{
	struct nss_crypto_ctrl_idx *ctrl_idx;
	struct nss_crypto_idx *idx;
	uint32_t paddr;
	int i;

	for (i = 0; i < NSS_CRYPTO_MAX_IDXS; i++) {
		ctrl_idx = &eng->idx_tbl[i];
		idx = &ctrl_idx->idx;
		paddr = 0;

		/*
		 * allocate the command block
		 */
		ctrl_idx->cblk = nss_crypto_desc_alloc(eng->dev, &paddr, sizeof(struct nss_crypto_cmd_block));
		if (!ctrl_idx->cblk) {
			nss_crypto_err("unable to allocate session table: idx no failed = %d\n", i);
			return NSS_CRYPTO_STATUS_ENOMEM;
		}


		idx->pp_num = (i % NSS_CRYPTO_BAM_PP);
		idx->cmd_len = NSS_CRYPTO_CACHE_CMD_SZ;

		/*
		 * for indexes beyond MAX_CACHED switch to uncached mode
		 */
		if ((i >= NSS_CRYPTO_MAX_CACHED_IDXS) || CONFIG_NSS_CRYPTO_FORCE_UNCACHE) {
			idx->cmd_len = NSS_CRYPTO_UNCACHE_CMD_SZ;
		}

		idx->cblk_paddr = paddr;

		/*
		 * finally update the index info into the message
		 */
		memcpy(&msg[i], idx, sizeof(struct nss_crypto_idx));
	}

	return NSS_CRYPTO_STATUS_OK;
}

/*
 * nss_crypto_ctrl_init()
 * 	initialize the crypto control
 */
void nss_crypto_ctrl_init(void)
{
	struct nss_crypto_ctrl *ctrl = &gbl_crypto_ctrl;
	struct nss_crypto_idx_info *idx_info;
	uint32_t idx;

	spin_lock_init(&ctrl->lock);

	mutex_init(&ctrl->mutex);

	ctrl->idx_bitmap = 0;
	ctrl->num_eng = 0;
	ctrl->num_idxs = 0;

	/*
	 * Initialize each session index timers used for
	 * session deletion requests management
	 */
	for (idx = 0; idx < NSS_CRYPTO_MAX_IDXS; idx++) {
		idx_info = &ctrl->idx_info[idx];
		init_timer(&idx_info->free_timer);
	}
}
