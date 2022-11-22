/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _NET_DSA_RTL83XX_H
#define _NET_DSA_RTL83XX_H __FILE__

#include <linux/dcache.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/netdevice.h>
#include <linux/notifier.h>
#include <linux/rhashtable-types.h>
#include <linux/types.h>
#include <net/dsa.h>
#include <uapi/linux/ethtool.h>
#include <uapi/linux/in6.h>

#define MAX_VLANS 4096
#define MAX_LAGS 16
#define MAX_PRIOS 8
#define RTL930X_PORT_IGNORE 0x3f
#define MAX_MC_GROUPS 512
#define UNKNOWN_MC_PMASK (MAX_MC_GROUPS - 1)
#define PIE_BLOCK_SIZE 128
#define MAX_PIE_ENTRIES (18 * PIE_BLOCK_SIZE)
#define N_FIXED_FIELDS 12
#define N_FIXED_FIELDS_RTL931X 14
#define MAX_COUNTERS 2048
#define MAX_ROUTES 512
#define MAX_HOST_ROUTES 1536
#define MAX_INTF_MTUS 8
#define DEFAULT_MTU 1536
#define MAX_INTERFACES 100
#define MAX_ROUTER_MACS 64
#define L3_EGRESS_DMACS 2048
#define MAX_SMACS 64

/* L3 actions */
#define L3_FORWARD		0
#define L3_DROP			1
#define L3_TRAP2CPU		2
#define L3_COPY2CPU		3
#define L3_TRAP2MASTERCPU	4
#define L3_COPY2MASTERCPU	5
#define L3_HARDDROP		6

/* PIE actions */
#define PIE_ACT_COPY_TO_PORT	2
#define PIE_ACT_REDIRECT_TO_PORT 4
#define PIE_ACT_ROUTE_UC	6
#define PIE_ACT_VID_ASSIGN	0

/* Route actions */
#define ROUTE_ACT_FORWARD	0
#define ROUTE_ACT_TRAP2CPU	1
#define ROUTE_ACT_COPY2CPU	2
#define ROUTE_ACT_DROP		3

/* SALRN Mode */
#define SALRN_PORT_SHIFT(p)			((p % 16) * 2)
#define SALRN_MODE_MASK				0x3
#define SALRN_MODE_HARDWARE			0
#define SALRN_MODE_DISABLED			2

/* Truncating distribution */
#define TRUNK_DISTRIBUTION_ALGO_SPA_BIT         0x01
#define TRUNK_DISTRIBUTION_ALGO_SMAC_BIT        0x02
#define TRUNK_DISTRIBUTION_ALGO_DMAC_BIT        0x04
#define TRUNK_DISTRIBUTION_ALGO_SIP_BIT         0x08
#define TRUNK_DISTRIBUTION_ALGO_DIP_BIT         0x10
#define TRUNK_DISTRIBUTION_ALGO_SRC_L4PORT_BIT  0x20
#define TRUNK_DISTRIBUTION_ALGO_DST_L4PORT_BIT  0x40
#define TRUNK_DISTRIBUTION_ALGO_MASKALL         0x7F

#define TRUNK_DISTRIBUTION_ALGO_L2_SPA_BIT         0x01
#define TRUNK_DISTRIBUTION_ALGO_L2_SMAC_BIT        0x02
#define TRUNK_DISTRIBUTION_ALGO_L2_DMAC_BIT        0x04
#define TRUNK_DISTRIBUTION_ALGO_L2_VLAN_BIT         0x08
#define TRUNK_DISTRIBUTION_ALGO_L2_MASKALL         0xF

#define TRUNK_DISTRIBUTION_ALGO_L3_SPA_BIT         0x01
#define TRUNK_DISTRIBUTION_ALGO_L3_SMAC_BIT        0x02
#define TRUNK_DISTRIBUTION_ALGO_L3_DMAC_BIT        0x04
#define TRUNK_DISTRIBUTION_ALGO_L3_VLAN_BIT         0x08
#define TRUNK_DISTRIBUTION_ALGO_L3_SIP_BIT         0x10
#define TRUNK_DISTRIBUTION_ALGO_L3_DIP_BIT         0x20
#define TRUNK_DISTRIBUTION_ALGO_L3_SRC_L4PORT_BIT  0x40
#define TRUNK_DISTRIBUTION_ALGO_L3_DST_L4PORT_BIT  0x80
#define TRUNK_DISTRIBUTION_ALGO_L3_PROTO_BIT  0x100
#define TRUNK_DISTRIBUTION_ALGO_L3_FLOW_LABEL_BIT  0x200
#define TRUNK_DISTRIBUTION_ALGO_L3_MASKALL         0x3FF

/* special port action controls */
/*  values:
 *      0 = FORWARD (default)
 *      1 = DROP
 *      2 = TRAP2CPU
 *      3 = FLOOD IN ALL PORT
 *
 *      Register encoding.
 *      offset = CTRL + (port >> 4) << 2
 *      value/mask = 3 << ((port & 0xF) << 1)
 */
typedef enum {
	FORWARD = 0,
	DROP,
	TRAP2CPU,
	FLOODALL,
	TRAP2MASTERCPU,
	COPY2CPU,
} action_type_t;

enum egr_filter {
	EGR_DISABLE = 0,
	EGR_ENABLE = 1,
};

enum fwd_rule_action {
	FWD_RULE_ACTION_NONE = 0,
	FWD_RULE_ACTION_FWD = 1,
};

enum l2_entry_type {
	L2_INVALID = 0,
	L2_UNICAST = 1,
	L2_MULTICAST = 2,
	IP4_MULTICAST = 3,
	IP6_MULTICAST = 4,
};

enum igr_filter {
	IGR_FORWARD = 0,
	IGR_DROP = 1,
	IGR_TRAP = 2,
};

enum pbvlan_mode {
	PBVLAN_MODE_UNTAG_AND_PRITAG = 0,
	PBVLAN_MODE_UNTAG_ONLY,
	PBVLAN_MODE_ALL_PKT,
};

enum pbvlan_type {
	PBVLAN_TYPE_INNER = 0,
	PBVLAN_TYPE_OUTER,
};

enum pie_phase {
	PHASE_VACL = 0,
	PHASE_IACL = 1,
};

enum phy_type {
	PHY_NONE = 0,
	PHY_RTL838X_SDS = 1,
	PHY_RTL8218B_INT = 2,
	PHY_RTL8218B_EXT = 3,
	PHY_RTL8214FC = 4,
	PHY_RTL839X_SDS = 5,
	PHY_RTL930X_SDS = 6,
};

typedef enum {
	BPDU = 0,
	PTP,
	PTP_UDP,
	PTP_ETH2,
	LLTP,
	EAPOL,
	GRATARP,
} rma_ctrl_t;

typedef enum {
	RTL8380_TBL_L2 = 0,
	RTL8380_TBL_0,
	RTL8380_TBL_1,
	RTL8390_TBL_L2,
	RTL8390_TBL_0,
	RTL8390_TBL_1,
	RTL8390_TBL_2,
	RTL9300_TBL_L2,
	RTL9300_TBL_0,
	RTL9300_TBL_1,
	RTL9300_TBL_2,
	RTL9300_TBL_HSB,
	RTL9300_TBL_HSA,
	RTL9310_TBL_0,
	RTL9310_TBL_1,
	RTL9310_TBL_2,
	RTL9310_TBL_3,
	RTL9310_TBL_4,
	RTL9310_TBL_5,
	RTL_TBL_END
} rtl838x_tbl_reg_t;

#define MIB_DESC(_size, _offset, _name) {.size = _size, .offset = _offset, .name = _name}
struct rtl83xx_mib_desc {
	unsigned int size;
	unsigned int offset;
	const char *name;
};

/* Intermediate representation of a  Packet Inspection Engine Rule
 * as suggested by the Kernel's tc flower offload subsystem
 * Field meaning is universal across SoC families, but data content is specific
 * to SoC family (e.g. because of different port ranges)
 */
struct pie_rule {
	int id;
	enum pie_phase phase;   /* Phase in which this template is applied */
	int packet_cntr;        /* ID of a packet counter assigned to this rule */
	int octet_cntr;         /* ID of a byte counter assigned to this rule */
	u32 last_packet_cnt;
	u64 last_octet_cnt;

	/* The following are requirements for the pie template */
	bool is_egress;
	bool is_ipv6;           /* This is a rule with IPv6 fields */

	/* Fixed fields that are always matched against on RTL8380 */
	u8 spmmask_fix;
	u8 spn;                 /* Source port number */
	bool stacking_port;     /* Source port is stacking port */
	bool mgnt_vlan;         /* Packet arrived on management VLAN */
	bool dmac_hit_sw;       /* The packet's destination MAC matches one of the device's */
	bool content_too_deep;  /* The content of the packet cannot be parsed: too many layers */
	bool not_first_frag;    /* Not the first IP fragment */
	u8 frame_type_l4;       /* 0: UDP, 1: TCP, 2: ICMP/ICMPv6, 3: IGMP */
	u8 frame_type;          /* 0: ARP, 1: L2 only, 2: IPv4, 3: IPv6 */
	bool otag_fmt;          /* 0: outer tag packet, 1: outer priority tag or untagged */
	bool itag_fmt;          /* 0: inner tag packet, 1: inner priority tag or untagged */
	bool otag_exist;        /* packet with outer tag */
	bool itag_exist;        /* packet with inner tag */
	bool frame_type_l2;     /* 0: Ethernet, 1: LLC_SNAP, 2: LLC_Other, 3: Reserved */
	bool igr_normal_port;   /* Ingress port is not cpu or stacking port */
	u8 tid;                 /* The template ID defining the what the templated fields mean */

	/* Masks for the fields that are always matched against on RTL8380 */
	u8 spmmask_fix_m;
	u8 spn_m;
	bool stacking_port_m;
	bool mgnt_vlan_m;
	bool dmac_hit_sw_m;
	bool content_too_deep_m;
	bool not_first_frag_m;
	u8 frame_type_l4_m;
	u8 frame_type_m;
	bool otag_fmt_m;
	bool itag_fmt_m;
	bool otag_exist_m;
	bool itag_exist_m;
	bool frame_type_l2_m;
	bool igr_normal_port_m;
	u8 tid_m;

	/* Logical operations between rules, special rules for rule numbers apply */
	bool valid;
	bool cond_not;          /* Matches when conditions not match */
	bool cond_and1;         /* And this rule 2n with the next rule 2n+1 in same block */
	bool cond_and2;         /* And this rule m in block 2n with rule m in block 2n+1 */
	bool ivalid;

	/* Actions to be performed */
	bool drop;              /* Drop the packet */
	bool fwd_sel;           /* Forward packet: to port, portmask, dest route, next rule, drop */
	bool ovid_sel;          /* So something to outer vlan-id: shift, re-assign */
	bool ivid_sel;          /* Do something to inner vlan-id: shift, re-assign */
	bool flt_sel;           /* Filter the packet when sending to certain ports */
	bool log_sel;           /* Log the packet in one of the LOG-table counters */
	bool rmk_sel;           /* Re-mark the packet, i.e. change the priority-tag */
	bool meter_sel;         /* Meter the packet, i.e. limit rate of this type of packet */
	bool tagst_sel;         /* Change the ergress tag */
	bool mir_sel;           /* Mirror the packet to a Link Aggregation Group */
	bool nopri_sel;         /* Change the normal priority */
	bool cpupri_sel;        /* Change the CPU priority */
	bool otpid_sel;         /* Change Outer Tag Protocol Identifier (802.1q) */
	bool itpid_sel;         /* Change Inner Tag Protocol Identifier (802.1q) */
	bool shaper_sel;        /* Apply traffic shaper */
	bool mpls_sel;          /* MPLS actions */
	bool bypass_sel;        /* Bypass actions */
	bool fwd_sa_lrn;        /* Learn the source address when forwarding */
	bool fwd_mod_to_cpu;    /* Forward the modified VLAN tag format to CPU-port */

	/* Fields used in predefined templates 0-2 on RTL8380 / 90 / 9300 */
	u64 spm;                /* Source Port Matrix */
	u16 otag;               /* Outer VLAN-ID */
	u8 smac[ETH_ALEN];      /* Source MAC address */
	u8 dmac[ETH_ALEN];      /* Destination MAC address */
	u16 ethertype;          /* Ethernet frame type field in ethernet header */
	u16 itag;               /* Inner VLAN-ID */
	u16 field_range_check;
	u32 sip;                /* Source IP */
	struct in6_addr sip6;   /* IPv6 Source IP */
	u32 dip;                /* Destination IP */
	struct in6_addr dip6;   /* IPv6 Destination IP */
	u16 tos_proto;          /* IPv4: TOS + Protocol fields, IPv6: Traffic class + next header */
	u16 sport;              /* TCP/UDP source port */
	u16 dport;              /* TCP/UDP destination port */
	u16 icmp_igmp;
	u16 tcp_info;
	u16 dsap_ssap;                  /* Destination / Source Service Access Point bytes (802.3) */

	u64 spm_m;
	u16 otag_m;
	u8 smac_m[ETH_ALEN];
	u8 dmac_m[ETH_ALEN];
	u8 ethertype_m;
	u16 itag_m;
	u16 field_range_check_m;
	u32 sip_m;
	struct in6_addr sip6_m;         /* IPv6 Source IP mask */
	u32 dip_m;
	struct in6_addr dip6_m;         /* IPv6 Destination IP mask */
	u16 tos_proto_m;
	u16 sport_m;
	u16 dport_m;
	u16 icmp_igmp_m;
	u16 tcp_info_m;
	u16 dsap_ssap_m;

	/* Data associated with actions */
	u8 fwd_act;                     /* Type of forwarding action */
	                                /* 0: permit, 1: drop, 2: copy to port id, 4: copy to portmask */
	                                /* 4: redirect to portid, 5: redirect to portmask */
	                                /* 6: route, 7: vlan leaky (only 8380) */
	u16 fwd_data;                   /* Additional data for forwarding action, e.g. destination port */
	u8 ovid_act;
	u16 ovid_data;                  /* Outer VLAN ID */
	u8 ivid_act;
	u16 ivid_data;                  /* Inner VLAN ID */
	u16 flt_data;                   /* Filtering data */
	u16 log_data;                   /* ID of packet or octet counter in LOG table, on RTL93xx */
	                                /* unnecessary since PIE-Rule-ID == LOG-counter-ID */
	bool log_octets;
	u8 mpls_act;                    /* MPLS action type */
	u16 mpls_lib_idx;               /* MPLS action data */

	u16 rmk_data;                   /* Data for remarking */
	u16 meter_data;                 /* ID of meter for bandwidth control */
	u16 tagst_data;
	u16 mir_data;
	u16 nopri_data;
	u16 cpupri_data;
	u16 otpid_data;
	u16 itpid_data;
	u16 shaper_data;

	/* Bypass actions, ignored on RTL8380 */
	bool bypass_all;                /* Not clear */
	bool bypass_igr_stp;            /* Bypass Ingress STP state */
	bool bypass_ibc_sc;             /* Bypass Ingress Bandwidth Control and Storm Control */
};

struct rtl838x_switch_priv;

struct rtl83xx_flow {
	unsigned long cookie;
	struct rhash_head node;
	struct rcu_head rcu_head;
	struct rtl838x_switch_priv *priv;
	struct pie_rule rule;
	u32 flags;
};

struct rtl93xx_route_attr {
	bool valid;
	bool hit;
	bool ttl_dec;
	bool ttl_check;
	bool dst_null;
	bool qos_as;
	u8 qos_prio;
	u8 type;
	u8 action;
};

struct rtl83xx_nexthop {
	u16 id;                         /* ID: L3_NEXT_HOP table-index or route-index set in L2_NEXT_HOP */
	u32 dev_id;
	u16 port;
	u16 vid;                        /* VLAN-ID for L2 table entry (saved from L2-UC entry) */
	u16 rvid;                       /* Relay VID/FID for the L2 table entry */
	u64 mac;                        /* The MAC address of the entry in the L2_NEXT_HOP table */
	u16 mac_id;
	u16 l2_id;                      /* Index of this next hop forwarding entry in L2 FIB table */
	u64 gw;                         /* The gateway MAC address packets are forwarded to */
	int if_id;                      /* Interface (into L3_EGR_INTF_IDX) */
};

/* An entry in the RTL93XX SoC's ROUTER_MAC tables setting up a termination point
 * for the L3 routing system. Packets arriving and matching an entry in this table
 * will be considered for routing.
 * Mask fields state whether the corresponding data fields matter for matching
 */
struct rtl93xx_rt_mac {
	bool valid;                     /* Valid or not */
	bool p_type;                    /* Individual (0) or trunk (1) port */
	bool p_mask;                    /* Whether the port type is used */
	u8 p_id;
	u8 p_id_mask;                   /* Mask for the port */
	u8 action;                      /* Routing action performed: 0: FORWARD, 1: DROP, 2: TRAP2CPU */
	                                /*   3: COPY2CPU, 4: TRAP2MASTERCPU, 5: COPY2MASTERCPU, 6: HARDDROP */
	u16 vid;
	u16 vid_mask;
	u64 mac;                        /* MAC address used as source MAC in the routed packet */
	u64 mac_mask;
};

struct rtl83xx_route {
	u32 gw_ip;                      /* IP of the route's gateway */
	u32 dst_ip;                     /* IP of the destination net */
	struct in6_addr dst_ip6;
	int prefix_len;                 /* Network prefix len of the destination net */
	bool is_host_route;
	int id;                         /* ID number of this route */
	struct rhlist_head linkage;
	u16 switch_mac_id;              /* Index into switch's own MACs, RTL839X only */
	struct rtl83xx_nexthop nh;
	struct pie_rule pr;
	struct rtl93xx_route_attr attr;
};

struct rtl838x_l2_entry {
	u8 mac[6];
	u16 vid;
	u16 rvid;
	u8 port;
	bool valid;
	enum l2_entry_type type;
	bool is_static;
	bool is_ip_mc;
	bool is_ipv6_mc;
	bool block_da;
	bool block_sa;
	bool suspended;
	bool next_hop;
	int age;
	u8 trunk;
	bool is_trunk;
	u8 stack_dev;
	u16 mc_portmask_index;
	u32 mc_gip;
	u32 mc_sip;
	u16 mc_mac_index;
	u16 nh_route_id;
	bool nh_vlan_target;            /* Only RTL83xx: VLAN used for next hop */

	/* The following is only valid on RTL931x */
	bool is_open_flow;
	bool is_pe_forward;
	bool is_local_forward;
	bool is_remote_forward;
	bool is_l2_tunnel;
	int l2_tunnel_id;
	int l2_tunnel_list_id;
};

struct rtl838x_vlan_info {
	u64 untagged_ports;
	u64 tagged_ports;
	u8 profile_id;
	bool hash_mc_fid;
	bool hash_uc_fid;
	u8 fid;                         /* AKA MSTI */

	/* The following fields are used only by the RTL931X */
	int if_id;                      /* Interface (index in L3_EGR_INTF_IDX) */
	u16 multicast_grp_mask;
	int l2_tunnel_list_id;
};

struct rtl838x_l3_intf {
	u16 vid;
	u8 smac_idx;
	u8 ip4_mtu_id;
	u8 ip6_mtu_id;
	u16 ip4_mtu;
	u16 ip6_mtu;
	u8 ttl_scope;
	u8 hl_scope;
	u8 ip4_icmp_redirect;
	u8 ip6_icmp_redirect;
	u8 ip4_pbr_icmp_redirect;
	u8 ip6_pbr_icmp_redirect;
};

struct rtl838x_reg {
	void (*mask_port_reg_be)(u64 clear, u64 set, int reg);
	void (*set_port_reg_be)(u64 set, int reg);
	u64 (*get_port_reg_be)(int reg);
	void (*mask_port_reg_le)(u64 clear, u64 set, int reg);
	void (*set_port_reg_le)(u64 set, int reg);
	u64 (*get_port_reg_le)(int reg);
	int stat_port_rst;
	int stat_rst;
	int stat_port_std_mib;
	int (*port_iso_ctrl)(int p);
	void (*traffic_enable)(int source, int dest);
	void (*traffic_disable)(int source, int dest);
	void (*traffic_set)(int source, u64 dest_matrix);
	u64 (*traffic_get)(int source);
	int l2_ctrl_0;
	int l2_ctrl_1;
	int smi_poll_ctrl;
	u32 l2_port_aging_out;
	int l2_tbl_flush_ctrl;
	void (*exec_tbl0_cmd)(u32 cmd);
	void (*exec_tbl1_cmd)(u32 cmd);
	int (*tbl_access_data_0)(int i);
	int isr_glb_src;
	void (*isr_port_link_sts_chg)(const u64 ports);
	void (*imr_port_link_sts_chg)(const u64 ports);
	int imr_glb;
	void (*vlan_tables_read)(u32 vlan, struct rtl838x_vlan_info *info);
	void (*vlan_set_tagged)(u32 vlan, struct rtl838x_vlan_info *info);
	void (*vlan_set_untagged)(u32 vlan, u64 portmask);
	void (*vlan_profile_dump)(int index);
	void (*vlan_profile_setup)(int profile);
	void (*vlan_port_pvidmode_set)(int port, enum pbvlan_type type, enum pbvlan_mode mode);
	void (*vlan_port_pvid_set)(int port, enum pbvlan_type type, int pvid);
	void (*vlan_port_keep_tag_set)(int port, bool keep_outer, bool keep_inner);
	void (*set_vlan_igr_filter)(int port, enum igr_filter state);
	void (*set_vlan_egr_filter)(int port, enum egr_filter state);
	void (*enable_learning)(int port, bool enable);
	void (*enable_flood)(int port, bool enable);
	void (*enable_mcast_flood)(int port, bool enable);
	void (*enable_bcast_flood)(int port, bool enable);
	void (*stp_get)(struct rtl838x_switch_priv *priv, u16 msti, u32 port_state[]);
	void (*stp_set)(struct rtl838x_switch_priv *priv, u16 msti, u32 port_state[]);
	int  (*mac_force_mode_ctrl)(int port);
	int  (*mac_port_ctrl)(int port);
	int  (*l2_port_new_salrn)(int port);
	int  (*l2_port_new_sa_fwd)(int port);
	int (*set_ageing_time)(unsigned long msec);
	int mir_ctrl;
	int mir_dpm;
	int mir_spm;
	int mac_link_sts;
	int mac_link_dup_sts;
	int  (*mac_link_spd_sts)(int port);
	int mac_rx_pause_sts;
	int mac_tx_pause_sts;
	u64 (*read_l2_entry_using_hash)(u32 hash, u32 position, struct rtl838x_l2_entry *e);
	void (*write_l2_entry_using_hash)(u32 hash, u32 pos, struct rtl838x_l2_entry *e);
	u64 (*read_cam)(int idx, struct rtl838x_l2_entry *e);
	void (*write_cam)(int idx, struct rtl838x_l2_entry *e);
	int (*trk_mbr_ctr)(int group);
	int rma_bpdu_fld_pmask;
	int spcl_trap_eapol_ctrl;
	void (*init_eee)(struct rtl838x_switch_priv *priv, bool enable);
	void (*port_eee_set)(struct rtl838x_switch_priv *priv, int port, bool enable);
	int (*eee_port_ability)(struct rtl838x_switch_priv *priv,
				struct ethtool_eee *e, int port);
	u64 (*l2_hash_seed)(u64 mac, u32 vid);
	u32 (*l2_hash_key)(struct rtl838x_switch_priv *priv, u64 seed);
	u64 (*read_mcast_pmask)(int idx);
	void (*write_mcast_pmask)(int idx, u64 portmask);
	void (*vlan_fwd_on_inner)(int port, bool is_set);
	void (*pie_init)(struct rtl838x_switch_priv *priv);
	int (*pie_rule_read)(struct rtl838x_switch_priv *priv, int idx, struct  pie_rule *pr);
	int (*pie_rule_write)(struct rtl838x_switch_priv *priv, int idx, struct pie_rule *pr);
	int (*pie_rule_add)(struct rtl838x_switch_priv *priv, struct pie_rule *rule);
	void (*pie_rule_rm)(struct rtl838x_switch_priv *priv, struct pie_rule *rule);
	void (*l2_learning_setup)(void);
	u32 (*packet_cntr_read)(int counter);
	void (*packet_cntr_clear)(int counter);
	void (*route_read)(int idx, struct rtl83xx_route *rt);
	void (*route_write)(int idx, struct rtl83xx_route *rt);
	void (*host_route_write)(int idx, struct rtl83xx_route *rt);
	int (*l3_setup)(struct rtl838x_switch_priv *priv);
	void (*set_l3_nexthop)(int idx, u16 dmac_id, u16 interface);
	void (*get_l3_nexthop)(int idx, u16 *dmac_id, u16 *interface);
	u64 (*get_l3_egress_mac)(u32 idx);
	void (*set_l3_egress_mac)(u32 idx, u64 mac);
	int (*find_l3_slot)(struct rtl83xx_route *rt, bool must_exist);
	int (*route_lookup_hw)(struct rtl83xx_route *rt);
	void (*get_l3_router_mac)(u32 idx, struct rtl93xx_rt_mac *m);
	void (*set_l3_router_mac)(u32 idx, struct rtl93xx_rt_mac *m);
	void (*set_l3_egress_intf)(int idx, struct rtl838x_l3_intf *intf);
	void (*set_distribution_algorithm)(int group, int algoidx, u32 algomask);
	void (*set_receive_management_action)(int port, rma_ctrl_t type, action_type_t action);
	void (*led_init)(struct rtl838x_switch_priv *priv);
};

struct rtl838x_port {
	bool enable;
	u64 pm;
	u16 pvid;
	bool eee_enabled;
	enum phy_type phy;
	bool phy_is_integrated;
	bool is10G;
	bool is2G5;
	int sds_num;
	int led_set;
	u32 led_num;
	const struct dsa_port *dp;
};

struct rtl838x_switch_priv {
	/* Switch operation */
	struct dsa_switch *ds;
	struct device *dev;
	u16 id;
	u16 family_id;
	char version;
	struct rtl838x_port ports[57];
	struct mutex reg_mutex;         /* Mutex for individual register manipulations */
	struct mutex pie_mutex;         /* Mutex for Packet Inspection Engine */
	int link_state_irq;
	int mirror_group_ports[4];
	struct mii_bus *mii_bus;
	const struct rtl838x_reg *r;
	u8 cpu_port;
	u8 port_mask;
	u8 port_width;
	u8 port_ignore;
	u64 irq_mask;
	u32 fib_entries;
	int l2_bucket_size;
	struct dentry *dbgfs_dir;
	int n_lags;
	u64 lags_port_members[MAX_LAGS];
	struct net_device *lag_devs[MAX_LAGS];
	u32 lag_primary[MAX_LAGS];
	u32 is_lagmember[57];
	u64 lagmembers;
	struct notifier_block nb;       /* TODO: change to different name */
	struct notifier_block ne_nb;
	struct notifier_block fib_nb;
	bool eee_enabled;
	unsigned long int mc_group_bm[MAX_MC_GROUPS >> 5];
	int mc_group_saves[MAX_MC_GROUPS];
	int n_pie_blocks;
	struct rhashtable tc_ht;
	unsigned long int pie_use_bm[MAX_PIE_ENTRIES >> 5];
	int n_counters;
	unsigned long int octet_cntr_use_bm[MAX_COUNTERS >> 5];
	unsigned long int packet_cntr_use_bm[MAX_COUNTERS >> 4];
	struct rhltable routes;
	unsigned long int route_use_bm[MAX_ROUTES >> 5];
	unsigned long int host_route_use_bm[MAX_HOST_ROUTES >> 5];
	struct rtl838x_l3_intf *interfaces[MAX_INTERFACES];
	u16 intf_mtus[MAX_INTF_MTUS];
	int intf_mtu_count[MAX_INTF_MTUS];
};

/* API for switch table access */
#define TBL_DESC(_addr, _data, _max_data, _c_bit, _t_bit, _rmode) \
		{  .addr = _addr, .data = _data, .max_data = _max_data, .c_bit = _c_bit, \
		    .t_bit = _t_bit, .rmode = _rmode \
		}
struct table_reg {
	u16 addr;
	u16 data;
	u8  max_data;
	u8 c_bit;
	u8 t_bit;
	u8 rmode;
	u8 tbl;
	struct mutex lock;
};

void rtl_table_init(void);
struct table_reg *rtl_table_get(rtl838x_tbl_reg_t r, int t);
void rtl_table_release(struct table_reg *r);
int rtl_table_read(struct table_reg *r, int idx);
int rtl_table_write(struct table_reg *r, int idx);
inline u16 rtl_table_data(struct table_reg *r, int i);
inline u32 rtl_table_data_r(struct table_reg *r, int i);
inline void rtl_table_data_w(struct table_reg *r, u32 v, int i);

void __init rtl83xx_setup_qos(struct rtl838x_switch_priv *priv);

int rtl83xx_packet_cntr_alloc(struct rtl838x_switch_priv *priv);

int rtl83xx_port_is_under(const struct net_device * dev, struct rtl838x_switch_priv *priv);

int read_phy(u32 port, u32 page, u32 reg, u32 *val);
int write_phy(u32 port, u32 page, u32 reg, u32 val);

/* Port register accessor functions for the RTL839x and RTL931X SoCs */
void rtl839x_mask_port_reg_be(u64 clear, u64 set, int reg);
u64 rtl839x_get_port_reg_be(int reg);
void rtl839x_set_port_reg_be(u64 set, int reg);
void rtl839x_mask_port_reg_le(u64 clear, u64 set, int reg);
void rtl839x_set_port_reg_le(u64 set, int reg);
u64 rtl839x_get_port_reg_le(int reg);

/* Port register accessor functions for the RTL838x and RTL930X SoCs */
void rtl838x_mask_port_reg(u64 clear, u64 set, int reg);
void rtl838x_set_port_reg(u64 set, int reg);
u64 rtl838x_get_port_reg(int reg);

int rtl83xx_dsa_phy_read(struct dsa_switch *ds, int phy_addr, int phy_reg);

int rtl83xx_lag_add(struct dsa_switch *ds, int group, int port, struct netdev_lag_upper_info *info);
int rtl83xx_lag_del(struct dsa_switch *ds, int group, int port);

#endif /* _NET_DSA_RTL83XX_H */
