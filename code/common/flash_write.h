/**

  @file    ioc_connection.h
  @brief   Connection object.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    5.8.2018

  A connection object represents logical connection between two devices. Both ends of
  communication have a connection object dedicated for that link, serialized data is
  transferred from a connection object to another.

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef IOC_CONNECTION_INCLUDED
#define IOC_CONNECTION_INCLUDED

/** Default iocom library socket port as string. This can be appended to IP address.
 */
// #define IOC_DEFAULT_SOCKET_PORT_STR "8369"
#define IOC_DEFAULT_SOCKET_PORT_STR OSAL_DEFAULT_SOCKET_PORT_STR


/** Frame sizes for socket and serial connections. These can never be modified, otherwise
 *  communication compatibility will break. Notice that socket frame size is not same as
 *  TCP frame size, signle tcp frame can hold multiple communication frames.
 */
/*@{*/
#define IOC_SOCKET_FRAME_SZ 464
#define IOC_SERIAL_FRAME_SZ 128
/*@}*/

/** Flags for ioc_connect() function. Bit fields.
 */
/*@{*/
#define IOC_SOCKET 1
#define IOC_CREATE_THREAD 2
#define IOC_CLOSE_CONNECTION_ON_ERROR 4
#define IOC_SERVER 8
#define IOC_DISABLE_SELECT 16
/*@}*/


/** Flags for message frame. Bit fields.
 */
/*@{*/
#define IOC_DELTA_ENCODED 1
#define IOC_COMPRESESSED 2
#define IOC_ADDR_HAS_TWO_BYTES 4
#define IOC_ADDR_HAS_FOUR_BYTES 8
#define IOC_HAS_DEVICE_NR 16
#define IOC_SYNC_COMPLETE 32
/*@}*/


/* Maximum parameter string length for end point.
 */
#define IOC_CONNECTION_PRMSTR_SZ 32


/**
****************************************************************************************************
    Member variables for frame being sent.
****************************************************************************************************
*/
typedef struct
{
    /** Pointer to outgoing frame buffer.
     */
    os_uchar *buf;

    /** Number of used bytes in buffer (current frame size). Zero if frame buffer is not used.
     */
    int used;

    /** Current send position within the buffer. 0 = at beginning of buffer.
     */
    int pos;

    /** Flags indication that outgoing frame buffer (buf) has been allocated by ioc_connect()
     */
    os_boolean allocated;

    /** Current frame count for serial communication frame enumeration.
     */
    os_uchar frame_count;
}
iocConnectionOutgoingFrame;


/**
****************************************************************************************************
    Member variables for incoming frame.
****************************************************************************************************
*/
typedef struct
{
    /** Pointer to infoming frame buffer.
     */
    os_uchar *buf;

    /** Current receive position within the buffer. 0 = at beginning of buffer.
     */
    int pos;

    /** Flags indication that incoming frame buffer (buf) has been allocated by ioc_connect()
     */
    os_boolean allocated;

    /** Current frame count for serial communication frame enumeration.
     */
    os_uchar frame_count;
}
iocConnectionIncomingFrame;


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************
    Worker thread specific member variables.
****************************************************************************************************
*/
typedef struct
{
    /** Event to activate the worker thread.
     */
    osalEvent trig;

    /** True If running worker thread for the end point.
     */
    os_boolean thread_running;

    /** Flag to terminate worker thread.
     */
    os_boolean stop_thread;
}
iocConnectionWorkerThread;
#endif


/**
****************************************************************************************************
    Linked list of connection's source buffers.
****************************************************************************************************
*/
typedef struct
{
    /** Pointer to connection's first source buffer in linked list.
     */
    struct iocSourceBuffer *first;

    /** Pointer to connection's last source buffer in linked list.
     */
    struct iocSourceBuffer *last;

    /** Connection buffer from which last send was done.
     */
    struct iocSourceBuffer *current;
}
iocConnectionsSourceBufferList;


/**
****************************************************************************************************
    Linked list of connection's target buffers.
****************************************************************************************************
*/
typedef struct
{
    /** Pointer to connection's first target buffer in linked list.
     */
    struct iocTargetBuffer *first;

    /** Pointer to connection's last target buffer in linked list.
     */
    struct iocTargetBuffer *last;
}
iocConnectionsTargetBufferList;


/**
****************************************************************************************************
    This connection in root's linked list of connections.
****************************************************************************************************
*/
typedef struct
{
    /** Pointer to the root object.
     */
    iocRoot *root;

    /** Pointer to the next connection in linked list.
     */
    struct iocConnection *next;

    /** Pointer to the previous connection in linked list.
     */
    struct iocConnection *prev;
}
iocConnectionLink;


/**
****************************************************************************************************

  @name Connection object structure.

  X...

****************************************************************************************************
*/
typedef struct iocConnection
{
#if OSAL_DEBUG
    /** Debug identifier must be first item in the object structure. It is used to verify
        that a function argument is pointer to correct initialized object.
     */
    int debug_id;
#endif
    /** Flags as given to ioc_listen()
     */
    int flags;

    /** Parameter string
     */
    os_char parameters[IOC_CONNECTION_PRMSTR_SZ];

    /** Total frame size, constant for connection type. For example IOC_SOCKET_FRAME_SZ
        for socket communication.
     */
    os_int frame_sz;

    /** OSAL stream handle (socket or serial port).
     */
    osalStream stream;

    /** Timer to measure how long since last failed stream open try.
        Zero if stream has not been tried or it has succeeded the
        last time.
     */
    os_timer socket_open_fail_timer;

    /** Timer of the last successfull receive.
     */
    os_timer last_receive;

    /** Timer of the last successfull send.
     */
    os_timer last_send;

    /** Member variables for current outgoing frame.
     */
    iocConnectionOutgoingFrame frame_out;

    /** Member variables for current incoming frame.
     */
    iocConnectionIncomingFrame frame_in;

    /** Number of received bytes since last connect.
     */
    os_ushort bytes_received;

    /** Number of bytes received by the other end
     *  of the connection (last received RBYTES value).
     */
    os_ushort processed_bytes;

#if OSAL_MULTITHREAD_SUPPORT
    /** Worker thread specific member variables.
     */
    iocConnectionWorkerThread worker;
#endif

    /** Linked list of connection's source buffers.
     */
    iocConnectionsSourceBufferList sbuf;

    /** Linked list of connection's target buffers.
     */
    iocConnectionsTargetBufferList tbuf;

    /** This connection in root's linked list of connections.
     */
    iocConnectionLink link;

    /** Flag indicating that the connection structure was dynamically allocated.
     */
    os_boolean allocated;
}
iocConnection;


/**
****************************************************************************************************

  @name Functions related to iocom root object

  The ioc_initialize_connection() function initializes or allocates new connection object,
  and ioc_release_connection() releases resources associated with it. Memory allocated for the
  connection is freed, if the memory was allocated by ioc_initialize_connection().

  The ioc_read() and ioc_write() functions are used to access data in connection.

****************************************************************************************************
 */
/*@{*/

/* Initialize connection object.
 */
iocConnection *ioc_initialize_connection(
    iocConnection *con,
    iocRoot *root);

/* Release connection object.
 */
void ioc_release_connection(
    iocConnection *con);

/* Start or prepare the connection.
 */
osalStatus ioc_connect(
    iocConnection *con,
    os_char *parameters,
    osalStream newsocket,
    os_uchar *frame_out_buf,
    int frame_out_buf_sz,
    os_uchar *frame_in_buf,
    int frame_in_buf_sz,
    int flags);

/* Connect and move data.
 */
osalStatus ioc_run_connection(
    iocConnection *con);

/* Request to terminate connection worker thread.
 */
osalStatus ioc_terminate_connection_thread(
    iocConnection *con);

/* Send frame to connection, if any available.
 */
osalStatus ioc_connection_send(
    iocConnection *con);

/* Send keep alive data to connection, if any available.
 */
osalStatus ioc_send_keepalive(
    iocConnection *con);

/* Receive data from connection.
 */
osalStatus ioc_connection_receive(
    iocConnection *con);

/*@}*/

#endif
