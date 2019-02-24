/*
 * Copyright (c) 2019 HabanaLabs Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "hlthunk_tests.h"
#include "uapi/misc/habanalabs.h"
#include "goya/goya.h"
#include "goya/goya_packets.h"

#include <stdio.h>
#include <string.h>

static uint32_t goya_add_nop_pkt(void *buffer, uint32_t buf_off, bool eb,
					bool mb)
{
	struct packet_nop packet = {0};

	packet.opcode = PACKET_NOP;
	packet.eng_barrier = eb;
	packet.msg_barrier = mb;
	packet.reg_barrier = 1;

	return hltests_add_packet_to_cb(buffer, buf_off, &packet,
						sizeof(packet));
}

static uint32_t goya_add_msg_long_pkt(void *buffer, uint32_t buf_off, bool eb,
					bool mb, uint64_t address,
					uint32_t value)
{
	struct packet_msg_long packet = {0};

	packet.opcode = PACKET_MSG_LONG;
	packet.addr = address;
	packet.value = value;
	packet.eng_barrier = eb;
	packet.msg_barrier = mb;
	packet.reg_barrier = 1;

	return hltests_add_packet_to_cb(buffer, buf_off, &packet,
						sizeof(packet));
}

static uint32_t goya_add_msg_short_pkt(void *buffer, uint32_t buf_off, bool eb,
					bool mb, uint16_t address,
					uint32_t value)
{
	struct packet_msg_short packet = {0};

	packet.opcode = PACKET_MSG_SHORT;
	packet.msg_addr_offset = address;
	packet.value = value;
	packet.eng_barrier = eb;
	packet.msg_barrier = mb;
	packet.reg_barrier = 1;

	return hltests_add_packet_to_cb(buffer, buf_off, &packet,
						sizeof(packet));
}

static uint32_t goya_add_arm_monitor_pkt(void *buffer, uint32_t buf_off,
					bool eb, bool mb, uint16_t address,
					uint32_t value, uint8_t mon_mode,
					uint16_t sync_val, uint16_t sync_id)
{
	struct packet_msg_short packet = {0};

	packet.opcode = PACKET_MSG_SHORT;
	packet.msg_addr_offset = address;
	packet.value = value;
	packet.eng_barrier = eb;
	packet.msg_barrier = mb;
	packet.reg_barrier = 1;
	packet.mon_arm_register.mode = mon_mode;
	packet.mon_arm_register.sync_value = sync_val;
	packet.mon_arm_register.sync_id = sync_id;

	return hltests_add_packet_to_cb(buffer, buf_off, &packet,
						sizeof(packet));
}

static uint32_t goya_add_fence_pkt(void *buffer, uint32_t buf_off, bool eb,
					bool mb, uint8_t dec_val,
					uint8_t gate_val, uint8_t fence_id)
{
	struct packet_fence packet = {0};

	packet.opcode = PACKET_FENCE;
	packet.dec_val = dec_val;
	packet.gate_val = gate_val;
	packet.id = fence_id;
	packet.eng_barrier = eb;
	packet.msg_barrier = mb;
	packet.reg_barrier = 1;

	return hltests_add_packet_to_cb(buffer, buf_off, &packet,
						sizeof(packet));
}

static uint32_t goya_add_dma_pkt(void *buffer, uint32_t buf_off, bool eb,
			bool mb, uint64_t src_addr,
			uint64_t dst_addr, uint32_t size,
			enum hltests_goya_dma_direction dma_dir)
{
	struct packet_lin_dma packet = {0};

	packet.opcode = PACKET_LIN_DMA;
	packet.eng_barrier = eb;
	packet.msg_barrier = mb;
	packet.reg_barrier = 1;
	packet.weakly_ordered = 1;
	packet.src_addr = src_addr;
	packet.dst_addr = dst_addr;
	packet.tsize = size;
	packet.dma_dir = dma_dir;

	return hltests_add_packet_to_cb(buffer, buf_off, &packet,
						sizeof(packet));
}

static uint32_t goya_tests_add_monitor_and_fence(void *buffer, uint32_t buf_off,
					uint8_t queue_id, bool cmdq_fence,
					uint32_t so_id, uint32_t mon_id,
					uint64_t mon_address)
{
	uint64_t address, monitor_base;
	uint32_t fence_addr = 0;
	uint16_t msg_addr_offset;

	switch (queue_id)
	{
	case GOYA_QUEUE_ID_DMA_0:
		fence_addr = mmDMA_QM_0_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_DMA_1:
		fence_addr = mmDMA_QM_1_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_DMA_2:
		fence_addr = mmDMA_QM_2_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_DMA_3:
		fence_addr = mmDMA_QM_3_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_DMA_4:
		fence_addr = mmDMA_QM_4_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_MME:
		if (cmdq_fence)
			fence_addr = mmMME_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmMME_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC0:
		if (cmdq_fence)
			fence_addr = mmTPC0_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC0_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC1:
		if (cmdq_fence)
			fence_addr = mmTPC1_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC1_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC2:
		if (cmdq_fence)
			fence_addr = mmTPC2_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC2_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC3:
		if (cmdq_fence)
			fence_addr = mmTPC3_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC3_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC4:
		if (cmdq_fence)
			fence_addr = mmTPC4_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC4_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC5:
		if (cmdq_fence)
			fence_addr = mmTPC5_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC5_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC6:
		if (cmdq_fence)
			fence_addr = mmTPC6_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC6_QM_CP_FENCE0_RDATA;
		break;
	case GOYA_QUEUE_ID_TPC7:
		if (cmdq_fence)
			fence_addr = mmTPC7_CMDQ_CP_FENCE0_RDATA;
		else
			fence_addr = mmTPC7_QM_CP_FENCE0_RDATA;
		break;
	default:
		printf("Failed to configure fence - invalid queue ID %d\n",
			queue_id);
	}

	if (mon_address)
		address = mon_address;
	else
		address = CFG_BASE + fence_addr;

	/* monitor_base should be the content of the base0 address registers,
	 * so it will be added to the msg short offsets
	 */
	monitor_base = mmSYNC_MNGR_MON_PAY_ADDRL_0;

	/* First monitor config packet: low address of the sync */
	msg_addr_offset = (mmSYNC_MNGR_MON_PAY_ADDRL_0 + mon_id * 4) -
								monitor_base;
	buf_off = goya_add_msg_short_pkt(buffer, buf_off, false, true,
					msg_addr_offset, (uint32_t) address);

	/* Second config packet: high address of the sync */
	msg_addr_offset = (mmSYNC_MNGR_MON_PAY_ADDRH_0 + mon_id * 4) -
								monitor_base;
	buf_off = goya_add_msg_short_pkt(buffer, buf_off, false, true,
				msg_addr_offset, (uint32_t) (address >> 32));

	/* Third config packet: the payload, i.e. what to write when the sync
	 * triggers
	 */
	msg_addr_offset = (mmSYNC_MNGR_MON_PAY_DATA_0 + mon_id * 4) -
								monitor_base;
	buf_off = goya_add_msg_short_pkt(buffer, buf_off, false, true,
					msg_addr_offset, 1);

	/* Fourth config packet: bind the monitor to a sync object */
	msg_addr_offset = (mmSYNC_MNGR_MON_ARM_0 + mon_id * 4) - monitor_base;
	buf_off = goya_add_arm_monitor_pkt(buffer, buf_off, false, true,
					msg_addr_offset, 1, 0, 1, so_id);

	/* Fence packet */
	buf_off = goya_add_fence_pkt(buffer, buf_off, false, true, 1, 1, 0);

	return buf_off;
}

static uint32_t goya_get_dma_down_qid(void)
{
	return GOYA_QUEUE_ID_DMA_1;
}

static uint32_t goya_get_dma_up_qid(void)
{
	return GOYA_QUEUE_ID_DMA_2;
}

static const struct hltests_asic_funcs goya_funcs = {
	.add_monitor_and_fence = goya_tests_add_monitor_and_fence,
	.add_nop_pkt = goya_add_nop_pkt,
	.add_msg_long_pkt = goya_add_msg_long_pkt,
	.add_msg_short_pkt = goya_add_msg_short_pkt,
	.add_arm_monitor_pkt = goya_add_arm_monitor_pkt,
	.add_fence_pkt = goya_add_fence_pkt,
	.add_dma_pkt = goya_add_dma_pkt,
	.get_dma_down_qid = goya_get_dma_down_qid,
	.get_dma_up_qid = goya_get_dma_up_qid,
};

void goya_tests_set_asic_funcs(struct hltests_device *hdev)
{
	hdev->asic_funcs = &goya_funcs;
}
