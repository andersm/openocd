/***************************************************************************
*   Copyright (C) 2005 by Dominic Rath                                    *
*   Dominic.Rath@gmx.de                                                   *
*                                                                         *
*   Copyright (C) 2007,2008 �yvind Harboe                                 *
*   oyvind.harboe@zylin.com                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#ifndef JTAG_H
#define JTAG_H

#include "binarybuffer.h"
#include "log.h"


#ifdef _DEBUG_JTAG_IO_
#define DEBUG_JTAG_IO(expr ...)		LOG_DEBUG(expr)
#else
#define DEBUG_JTAG_IO(expr ...)
#endif

#ifndef DEBUG_JTAG_IOZ
#define DEBUG_JTAG_IOZ 64
#endif

/*-----<Macros>--------------------------------------------------*/

/**
 * When given an array, compute its DIMension; in other words, the
 * number of elements in the array
 */
#define DIM(x)					(sizeof(x)/sizeof((x)[0]))

/** Calculate the number of bytes required to hold @a n TAP scan bits */
#define TAP_SCAN_BYTES(n)		CEIL(n, 8)

/*-----</Macros>-------------------------------------------------*/

/**
 * Defines JTAG Test Access Port states.
 *
 * These definitions were gleaned from the ARM7TDMI-S Technical
 * Reference Manual and validated against several other ARM core
 * technical manuals.  tap_get_tms_path() is sensitive to this numbering
 * and ordering of the TAP states; furthermore, some interfaces require
 * specific numbers be used, as they are handed-off directly to their
 * hardware implementations.
 */
typedef enum tap_state
{
#if BUILD_ECOSBOARD
	/* These are the old numbers. Leave as-is for now... */
	TAP_RESET    = 0, TAP_IDLE = 8,
	TAP_DRSELECT = 1, TAP_DRCAPTURE = 2, TAP_DRSHIFT = 3, TAP_DREXIT1 = 4,
	TAP_DRPAUSE  = 5, TAP_DREXIT2 = 6, TAP_DRUPDATE = 7,
	TAP_IRSELECT = 9, TAP_IRCAPTURE = 10, TAP_IRSHIFT = 11, TAP_IREXIT1 = 12,
	TAP_IRPAUSE  = 13, TAP_IREXIT2 = 14, TAP_IRUPDATE = 15,

	TAP_NUM_STATES = 16, TAP_INVALID = -1,
#else
	/* Proper ARM recommended numbers */
	TAP_DREXIT2 = 0x0,
	TAP_DREXIT1 = 0x1,
	TAP_DRSHIFT = 0x2,
	TAP_DRPAUSE = 0x3,
	TAP_IRSELECT = 0x4,
	TAP_DRUPDATE = 0x5,
	TAP_DRCAPTURE = 0x6,
	TAP_DRSELECT = 0x7,
	TAP_IREXIT2 = 0x8,
	TAP_IREXIT1 = 0x9,
	TAP_IRSHIFT = 0xa,
	TAP_IRPAUSE = 0xb,
	TAP_IDLE = 0xc,
	TAP_IRUPDATE = 0xd,
	TAP_IRCAPTURE = 0xe,
	TAP_RESET = 0x0f,

	TAP_NUM_STATES = 0x10,

	TAP_INVALID = -1,
#endif
} tap_state_t;

/**
 * Function tap_state_name
 * Returns a string suitable for display representing the JTAG tap_state
 */
const char* tap_state_name(tap_state_t state);

/// The current TAP state of the pending JTAG command queue.
extern tap_state_t cmd_queue_cur_state;
/// The TAP state in which DR scans should end.
extern tap_state_t cmd_queue_end_state;

/**
 * This structure defines a single scan field in the scan. It provides
 * fields for the field's width and pointers to scan input and output
 * values.
 *
 * In addition, this structure includes a value and mask that is used by
 * jtag_add_dr_scan_check() to validate the value that was scanned out.
 *
 * The allocated, modified, and intmp fields are internal work space.
 */
typedef struct scan_field_s
{
	/// A pointer to the tap structure to which this field refers.
	jtag_tap_t* tap;

	/// The number of bits this field specifies (up to 32)
	int num_bits;
	/// A pointer to value to be scanned into the device
	u8* out_value;
	/// A pointer to a 32-bit memory location for data scanned out
	u8* in_value;

	/// The value used to check the data scanned out.
	u8* check_value;
	/// The mask to go with check_value
	u8* check_mask;

	/// in_value has been allocated for the queue
	int allocated;
	/// Indicates we modified the in_value.
	int modified;
	/// temporary storage for performing value checks synchronously
	u8 intmp[4];
} scan_field_t;

#ifdef INCLUDE_JTAG_INTERFACE_H

/**
 * The inferred type of a scan_command_s structure, indicating whether
 * the command has the host scan in from the device, the host scan out
 * to the device, or both.
 */
enum scan_type {
	/// From device to host,
	SCAN_IN = 1,
	/// From host to device,
	SCAN_OUT = 2,
	/// Full-duplex scan.
	SCAN_IO = 3
};

/**
 * The scan_command provide a means of encapsulating a set of scan_field_s
 * structures that should be scanned in/out to the device.
 */
typedef struct scan_command_s
{
	/// instruction/not data scan
	bool ir_scan;
	/// number of fields in *fields array
	int num_fields;
	/// pointer to an array of data scan fields
	scan_field_t* fields;
	/// state in which JTAG commands should finish
	tap_state_t end_state;
} scan_command_t;

typedef struct statemove_command_s
{
	/// state in which JTAG commands should finish
	tap_state_t end_state;
} statemove_command_t;

typedef struct pathmove_command_s
{
	/// number of states in *path
	int num_states;
	/// states that have to be passed
	tap_state_t* path;
} pathmove_command_t;

typedef struct runtest_command_s
{
	/// number of cycles to spend in Run-Test/Idle state
	int num_cycles;
	/// state in which JTAG commands should finish
	tap_state_t end_state;
} runtest_command_t;


typedef struct stableclocks_command_s
{
	/// number of clock cycles that should be sent
	int num_cycles;
} stableclocks_command_t;


typedef struct reset_command_s
{
	/// Set TRST output: 0=deassert, 1=assert, -1=no change
	int trst;
	/// Set SRST output: 0=deassert, 1=assert, -1=no change
	int srst;
} reset_command_t;

typedef struct end_state_command_s
{
	/// state in which JTAG commands should finish
	tap_state_t end_state;
} end_state_command_t;

typedef struct sleep_command_s
{
	/// number of microseconds to sleep
	u32 us;
} sleep_command_t;

/**
 * Defines a container type that hold a pointer to a JTAG command
 * structure of any defined type.
 */
typedef union jtag_command_container_u
{
	scan_command_t*         scan;
	statemove_command_t*    statemove;
	pathmove_command_t*     pathmove;
	runtest_command_t*      runtest;
	stableclocks_command_t* stableclocks;
	reset_command_t*        reset;
	end_state_command_t*    end_state;
	sleep_command_t* sleep;
} jtag_command_container_t;

/**
 * The type of the @c jtag_command_container_u contained by a
 * @c jtag_command_s structure.
 */
enum jtag_command_type {
	JTAG_SCAN         = 1,
	JTAG_STATEMOVE    = 2,
	JTAG_RUNTEST      = 3,
	JTAG_RESET        = 4,
	JTAG_PATHMOVE     = 6,
	JTAG_SLEEP        = 7,
	JTAG_STABLECLOCKS = 8
};

typedef struct jtag_command_s
{
	jtag_command_container_t cmd;
	enum jtag_command_type   type;
	struct jtag_command_s*   next;
} jtag_command_t;

/// The current queue of jtag_command_s structures.
extern jtag_command_t* jtag_command_queue;

extern void* cmd_queue_alloc(size_t size);
extern void cmd_queue_free(void);

extern void jtag_queue_command(jtag_command_t *cmd);
extern void jtag_command_queue_reset(void);

#endif // INCLUDE_JTAG_INTERFACE_H

typedef struct jtag_tap_event_action_s jtag_tap_event_action_t;

/* this is really: typedef jtag_tap_t */
/* But - the typedef is done in "types.h" */
/* due to "forward decloration reasons" */
struct jtag_tap_s
{
	const char* chip;
	const char* tapname;
	const char* dotted_name;
	int abs_chain_position;
	/// Is this TAP enabled?
	int enabled;
	int ir_length; /**< size of instruction register */
	u32 ir_capture_value;
	u8* expected; /**< Capture-IR expected value */
	u32 ir_capture_mask;
	u8* expected_mask; /**< Capture-IR expected mask */
	u32 idcode;
	/**< device identification code */

	/// Array of expected identification codes */
	u32* expected_ids;
	/// Number of expected identification codes
	u8 expected_ids_cnt;

	/// current instruction
	u8* cur_instr;
	/// Bypass register selected
	int bypass;

	jtag_tap_event_action_t *event_action;

	jtag_tap_t* next_tap;
};
extern jtag_tap_t* jtag_AllTaps(void);
extern jtag_tap_t* jtag_TapByPosition(int n);
extern jtag_tap_t* jtag_TapByString(const char* dotted_name);
extern jtag_tap_t* jtag_TapByJimObj(Jim_Interp* interp, Jim_Obj* obj);
extern jtag_tap_t* jtag_TapByAbsPosition(int abs_position);
extern int jtag_NumEnabledTaps(void);
extern int jtag_NumTotalTaps(void);

static __inline__ jtag_tap_t* jtag_NextEnabledTap(jtag_tap_t* p)
{
	if (p == NULL)
	{
		/* start at the head of list */
		p = jtag_AllTaps();
	}
	else
	{
		/* start *after* this one */
		p = p->next_tap;
	}
	while (p)
	{
		if (p->enabled)
		{
			break;
		}
		else
		{
			p = p->next_tap;
		}
	}

	return p;
}


enum reset_line_mode {
	LINE_OPEN_DRAIN = 0x0,
	LINE_PUSH_PULL  = 0x1,
};

enum jtag_event {
	JTAG_TRST_ASSERTED
};

extern char* jtag_event_strings[];

enum jtag_tap_event {
	JTAG_TAP_EVENT_ENABLE,
	JTAG_TAP_EVENT_DISABLE
};

extern const Jim_Nvp nvp_jtag_tap_event[];

struct jtag_tap_event_action_s
{
	enum jtag_tap_event      event;
	Jim_Obj*                 body;
	jtag_tap_event_action_t* next;
};

extern int jtag_trst;
extern int jtag_srst;

typedef struct jtag_event_callback_s
{
	int (*callback)(enum jtag_event event, void* priv);
	void*                         priv;
	struct jtag_event_callback_s* next;
} jtag_event_callback_t;

extern jtag_event_callback_t* jtag_event_callbacks;

extern int jtag_speed;
extern int jtag_speed_post_reset;

enum reset_types {
	RESET_NONE            = 0x0,
	RESET_HAS_TRST        = 0x1,
	RESET_HAS_SRST        = 0x2,
	RESET_TRST_AND_SRST   = 0x3,
	RESET_SRST_PULLS_TRST = 0x4,
	RESET_TRST_PULLS_SRST = 0x8,
	RESET_TRST_OPEN_DRAIN = 0x10,
	RESET_SRST_PUSH_PULL  = 0x20,
};

extern enum reset_types jtag_reset_config;

/**
 * Initialize interface upon startup.  Return a successful no-op upon
 * subsequent invocations.
 */
extern int  jtag_interface_init(struct command_context_s* cmd_ctx);

/// Shutdown the JTAG interface upon program exit.
extern int  jtag_interface_quit(void);

/**
 * Initialize JTAG chain using only a RESET reset. If init fails,
 * try reset + init.
 */
extern int  jtag_init(struct command_context_s* cmd_ctx);

/// reset, then initialize JTAG chain
extern int  jtag_init_reset(struct command_context_s* cmd_ctx);
extern int  jtag_register_commands(struct command_context_s* cmd_ctx);

/**
 * @file
 * The JTAG interface can be implemented with a software or hardware fifo.
 *
 * TAP_DRSHIFT and TAP_IRSHIFT are illegal end states; however,
 * TAP_DRSHIFT/IRSHIFT can be emulated as end states, by using longer
 * scans.
 *
 * Code that is relatively insensitive to the path taken through state
 * machine (as long as it is JTAG compliant) can use @a endstate for
 * jtag_add_xxx_scan(). Otherwise, the pause state must be specified as
 * end state and a subsequent jtag_add_pathmove() must be issued.
 */

extern void jtag_add_ir_scan(int num_fields, scan_field_t* fields, tap_state_t endstate);
/**
 * The same as jtag_add_ir_scan except no verification is performed out
 * the output values.
 */
extern void jtag_add_ir_scan_noverify(int num_fields, const scan_field_t *fields, tap_state_t state);


/**
 * Set in_value to point to 32 bits of memory to scan into. This
 * function is a way to handle the case of synchronous and asynchronous
 * JTAG queues.
 *
 * In the event of an asynchronous queue execution the queue buffer
 * allocation method is used, for the synchronous case the temporary 32
 * bits come from the input field itself.
 */
extern void jtag_alloc_in_value32(scan_field_t *field);

extern void jtag_add_dr_scan(int num_fields, const scan_field_t* fields, tap_state_t endstate);
/// A version of jtag_add_dr_scan() that uses the check_value/mask fields
extern void jtag_add_dr_scan_check(int num_fields, scan_field_t* fields, tap_state_t endstate);
extern void jtag_add_plain_ir_scan(int num_fields, const scan_field_t* fields, tap_state_t endstate);
extern void jtag_add_plain_dr_scan(int num_fields, const scan_field_t* fields, tap_state_t endstate);


/**
 * Defines a simple JTAG callback that can allow conversions on data
 * scanned in from an interface.
 *
 * This callback should only be used for conversion that cannot fail.
 * For conversion types or checks that can fail, use the more complete
 * variant: jtag_callback_t.
 */
typedef void (*jtag_callback1_t)(u8 *in);

/// A simpler version of jtag_add_callback4().
extern void jtag_add_callback(jtag_callback1_t, u8 *in);


/**
 * Defines the type of data passed to the jtag_callback_t interface.
 * The underlying type must allow storing an @c int or pointer type.
 */
typedef intptr_t jtag_callback_data_t;

/**
 * Defines the interface of the JTAG callback mechanism.
 *
 * @param in the pointer to the data clocked in
 * @param data1 An integer big enough to use as an @c int or a pointer.
 * @param data2 An integer big enough to use as an @c int or a pointer.
 * @param data3 An integer big enough to use as an @c int or a pointer.
 * @returns an error code
 */
typedef int (*jtag_callback_t)(u8 *in, jtag_callback_data_t data1, jtag_callback_data_t data2, jtag_callback_data_t data3);


/**
 * This callback can be executed immediately the queue has been flushed.
 *
 * The JTAG queue can be executed synchronously or asynchronously.
 * Typically for USB, the queue is executed asynchronously.  For
 * low-latency interfaces, the queue may be executed synchronously.
 *
 * The callback mechanism is very general and does not make many
 * assumptions about what the callback does or what its arguments are.
 * These callbacks are typically executed *after* the *entire* JTAG
 * queue has been executed for e.g. USB interfaces, and they are
 * guaranteeed to be invoked in the order that they were queued.
 *
 * If the execution of the queue fails before the callbacks, then --
 * depending on driver implementation -- the callbacks may or may not be
 * invoked.  @todo Can we make this behavior consistent?
 *
 * The strange name is due to C's lack of overloading using function
 * arguments.
 *
 * @param f The callback function to add.
 * @param in Typically used to point to the data to operate on.
 * Frequently this will be the data clocked in during a shift operation.
 * @param data1 An integer big enough to use as an @c int or a pointer.
 * @param data2 An integer big enough to use as an @c int or a pointer.
 * @param data3 An integer big enough to use as an @c int or a pointer.
 *
 */
extern void jtag_add_callback4(jtag_callback_t f, u8 *in,
		jtag_callback_data_t data1, jtag_callback_data_t data2,
		jtag_callback_data_t data3);


/**
 * Run a TAP_RESET reset where the end state is TAP_RESET,
 * regardless of the start state.
 */
extern void jtag_add_tlr(void);

/**
 * Application code *must* assume that interfaces will
 * implement transitions between states with different
 * paths and path lengths through the state diagram. The
 * path will vary across interface and also across versions
 * of the same interface over time. Even if the OpenOCD code
 * is unchanged, the actual path taken may vary over time
 * and versions of interface firmware or PCB revisions.
 *
 * Use jtag_add_pathmove() when specific transition sequences
 * are required.
 *
 * Do not use jtag_add_pathmove() unless you need to, but do use it
 * if you have to.
 *
 * DANGER! If the target is dependent upon a particular sequence
 * of transitions for things to work correctly(e.g. as a workaround
 * for an errata that contradicts the JTAG standard), then pathmove
 * must be used, even if some jtag interfaces happen to use the
 * desired path. Worse, the jtag interface used for testing a
 * particular implementation, could happen to use the "desired"
 * path when transitioning to/from end
 * state.
 *
 * A list of unambigious single clock state transitions, not
 * all drivers can support this, but it is required for e.g.
 * XScale and Xilinx support
 *
 * Note! TAP_RESET must not be used in the path!
 *
 * Note that the first on the list must be reachable
 * via a single transition from the current state.
 *
 * All drivers are required to implement jtag_add_pathmove().
 * However, if the pathmove sequence can not be precisely
 * executed, an interface_jtag_add_pathmove() or jtag_execute_queue()
 * must return an error. It is legal, but not recommended, that
 * a driver returns an error in all cases for a pathmove if it
 * can only implement a few transitions and therefore
 * a partial implementation of pathmove would have little practical
 * application.
 */
extern void jtag_add_pathmove(int num_states, const tap_state_t* path);

/**
 * Goes to TAP_IDLE (if we're not already there), cycle
 * precisely num_cycles in the TAP_IDLE state, after which move
 * to @a endstate (unless it is also TAP_IDLE).
 *
 * @param num_cycles Number of cycles in TAP_IDLE state.  This argument
 * 	may be 0, in which case this routine will navigate to @a endstate
 * 	via TAP_IDLE.
 * @param endstate The final state.
 */
extern void jtag_add_runtest(int num_cycles, tap_state_t endstate);

/**
 * A reset of the TAP state machine can be requested.
 *
 * Whether tms or trst reset is used depends on the capabilities of
 * the target and jtag interface(reset_config  command configures this).
 *
 * srst can driver a reset of the TAP state machine and vice
 * versa
 *
 * Application code may need to examine value of jtag_reset_config
 * to determine the proper codepath
 *
 * DANGER! Even though srst drives trst, trst might not be connected to
 * the interface, and it might actually be *harmful* to assert trst in this case.
 *
 * This is why combinations such as "reset_config srst_only srst_pulls_trst"
 * are supported.
 *
 * only req_tlr_or_trst and srst can have a transition for a
 * call as the effects of transitioning both at the "same time"
 * are undefined, but when srst_pulls_trst or vice versa,
 * then trst & srst *must* be asserted together.
 */
extern void jtag_add_reset(int req_tlr_or_trst, int srst);

extern void jtag_add_end_state(tap_state_t endstate);
extern void jtag_add_sleep(u32 us);


/**
 * Function jtag_add_stable_clocks
 * first checks that the state in which the clocks are to be issued is
 * stable, then queues up clock_count clocks for transmission.
 */
void jtag_add_clocks(int num_cycles);


/**
 * For software FIFO implementations, the queued commands can be executed
 * during this call or earlier. A sw queue might decide to push out
 * some of the jtag_add_xxx() operations once the queue is "big enough".
 *
 * This fn will return an error code if any of the prior jtag_add_xxx()
 * calls caused a failure, e.g. check failure. Note that it does not
 * matter if the operation was executed *before* jtag_execute_queue(),
 * jtag_execute_queue() will still return an error code.
 *
 * All jtag_add_xxx() calls that have in_handler!=NULL will have been
 * executed when this fn returns, but if what has been queued only
 * clocks data out, without reading anything back, then JTAG could
 * be running *after* jtag_execute_queue() returns. The API does
 * not define a way to flush a hw FIFO that runs *after*
 * jtag_execute_queue() returns.
 *
 * jtag_add_xxx() commands can either be executed immediately or
 * at some time between the jtag_add_xxx() fn call and jtag_execute_queue().
 */
extern int jtag_execute_queue(void);

/* same as jtag_execute_queue() but does not clear the error flag */
extern void jtag_execute_queue_noclear(void);

/**
 * The jtag_error variable is set when an error occurs while executing
 * the queue.
 *
 * This flag can also be set from application code, if an error happens
 * during processing that should be reported during jtag_execute_queue().
 *
 * It is cleared by jtag_execute_queue().
 */
extern int jtag_error;

static __inline__ void jtag_set_error(int error)
{
	if ((error==ERROR_OK)||(jtag_error!=ERROR_OK))
	{
		/* keep first error */
		return;
	}
	jtag_error=error;
}



/* can be implemented by hw+sw */
extern int jtag_power_dropout(int* dropout);
extern int jtag_srst_asserted(int* srst_asserted);

/* JTAG support functions */

/**
 * Execute jtag queue and check value with an optional mask.
 * @param field Pointer to scan field.
 * @param value Pointer to scan value.
 * @param mask Pointer to scan mask; may be NULL.
 * @returns Nothing, but calls jtag_set_error() on any error.
 */
extern void jtag_check_value_mask(scan_field_t *field, u8 *value, u8 *mask);

#ifdef INCLUDE_JTAG_INTERFACE_H
extern enum scan_type jtag_scan_type(const scan_command_t* cmd);
extern int jtag_scan_size(const scan_command_t* cmd);
extern int jtag_read_buffer(u8* buffer, const scan_command_t* cmd);
extern int jtag_build_buffer(const scan_command_t* cmd, u8** buffer);
#endif // INCLUDE_JTAG_INTERFACE_H

extern void jtag_sleep(u32 us);
extern int jtag_call_event_callbacks(enum jtag_event event);
extern int jtag_register_event_callback(int (* callback)(enum jtag_event event, void* priv), void* priv);

extern int jtag_verify_capture_ir;

void jtag_tap_handle_event(jtag_tap_t* tap, enum jtag_tap_event e);

/*
 * The JTAG subsystem defines a number of error codes,
 * using codes between -100 and -199.
 */
#define ERROR_JTAG_INIT_FAILED       (-100)
#define ERROR_JTAG_INVALID_INTERFACE (-101)
#define ERROR_JTAG_NOT_IMPLEMENTED   (-102)
#define ERROR_JTAG_TRST_ASSERTED     (-103)
#define ERROR_JTAG_QUEUE_FAILED      (-104)
#define ERROR_JTAG_NOT_STABLE_STATE  (-105)
#define ERROR_JTAG_DEVICE_ERROR      (-107)

/**
 * jtag_add_dr_out() is a version of jtag_add_dr_scan() which
 * only scans data out. It operates on 32 bit integers instead
 * of 8 bit, which makes it a better impedance match with
 * the calling code which often operate on 32 bit integers.
 *
 * Current or end_state can not be TAP_RESET. end_state can be TAP_INVALID
 *
 * num_bits[i] is the number of bits to clock out from value[i] LSB first.
 *
 * If the device is in bypass, then that is an error condition in
 * the caller code that is not detected by this fn, whereas
 * jtag_add_dr_scan() does detect it. Similarly if the device is not in
 * bypass, data must be passed to it.
 *
 * If anything fails, then jtag_error will be set and jtag_execute() will
 * return an error. There is no way to determine if there was a failure
 * during this function call.
 *
 * This is an inline fn to speed up embedded hosts. Also note that
 * interface_jtag_add_dr_out() can be a *small* inline function for
 * embedded hosts.
 *
 * There is no jtag_add_dr_outin() version of this fn that also allows
 * clocking data back in. Patches gladly accepted!
 */
extern void jtag_add_dr_out(jtag_tap_t* tap,
		int num_fields, const int* num_bits, const u32* value,
		tap_state_t end_state);


/**
 * jtag_add_statemove() moves from the current state to @a goal_state.
 *
 * This function was originally designed to handle the XSTATE command
 * from the XSVF specification.
 *
 * @param goal_state The final TAP state.
 * @return ERROR_OK on success, or an error code on failure.
 */
extern int jtag_add_statemove(tap_state_t goal_state);

#endif /* JTAG_H */
