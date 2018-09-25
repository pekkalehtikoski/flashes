/**

  @file    ioc_connection.c
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
#include "iocom/iocom.h"


/* Forward referred static functions.
 */
static osalStatus ioc_try_to_connect(
    iocConnection *con);

static void ioc_reset_connection_state(
    iocConnection *con);

static void ioc_connection_thread(
    void *prm,
    osalEvent done);


/**
****************************************************************************************************

  @brief Initialize connection.
  @anchor ioc_initialize_connection

  The ioc_initialize_connection() function initializes a connection. A connection can always
  be allocated global variable. In this case pointer to memory to be initialized is given as
  argument and return value is the same pointer. If dynamic memory allocation is supported,
  and the con argument is OS_NULL, the connection object is allocated by the function.

  @param   con Pointer to static connection structure, or OS_NULL to allocate connection
           object dynamically.
  @param   root Pointer to the root object.
  @return  Pointer to initialized connection object.

****************************************************************************************************
*/
iocConnection *ioc_initialize_connection(
    iocConnection *con,
    iocRoot *root)
{
    /* Check that root object is valid pointer.
     */
    osal_debug_assert(root->debug_id == 'R');

    if (con == OS_NULL)
    {
        con = (iocConnection*)os_malloc(sizeof(iocConnection), OS_NULL);
        os_memclear(con, sizeof(iocConnection));
        con->allocated = OS_TRUE;
    }
    else
    {
        os_memclear(con, sizeof(iocConnection));
    }

    /* Syncronize.
     */
    ioc_lock(root);

    /* Save pointer to root object and join to linked list of connections.
     */
    con->link.root = root;
    con->link.prev = root->con.last;
    if (root->con.last)
    {
        root->con.last->link.next = con;
    }
    else
    {
        root->con.first = con;
    }
    root->con.last = con;

#if OSAL_DEBUG
    /* Mark connection structure as initialized connection object (for debugging).
     */
    con->debug_id = 'C';
#endif

    /* End syncronization.
     */
    ioc_unlock(root);

    /* Return pointer to initialized connection.
     */
    osal_trace("connection: initialized");
    return con;
}


/**
****************************************************************************************************

  @brief Release connection.
  @anchor ioc_release_connection

  The ioc_release_connection() function releases resources allocated for the connection
  object. Memory allocated for the connection object is freed, if it was allocated by
  ioc_initialize_connection().

  @param   con Pointer to the connection object.
  @return  None.

****************************************************************************************************
*/
void ioc_release_connection(
    iocConnection *con)
{
    iocRoot
        *root;

    /* Check that con is valid pointer.
     */
    osal_debug_assert(con->debug_id == 'C');

    /* Syncronize.
     */
    root = con->link.root;
    ioc_lock(root);

    /* If stream is open, close it.
     */
    if (con->stream)
    {
        osal_debug_error("stream closed");
        osal_stream_close(con->stream);
        con->stream = OS_NULL;
    }

    /* Release all source buffers.
     */
    while (con->sbuf.first)
    {
        ioc_release_source_buffer(con->sbuf.first);
    }

    /* Release all target buffers.
     */
    while (con->tbuf.first)
    {
        ioc_release_target_buffer(con->tbuf.first);
    }

    /* Remove connection from linked list.
     */
    if (con->link.prev)
    {
        con->link.prev->link.next = con->link.next;
    }
    else
    {
        con->link.root->con.first = con->link.next;
    }
    if (con->link.next)
    {
        con->link.next->link.prev = con->link.prev;
    }
    else
    {
        con->link.root->con.last = con->link.prev;
    }

    /* Clear allocated memory indicate that is no longer initialized (for debugging and
       for primitive static allocation schema).
     */
    os_memclear(con, sizeof(iocConnection));

    /* End syncronization.
     */
    ioc_unlock(root);

    if (con->frame_out.allocated)
    {
        os_free(con->frame_out.buf, con->frame_sz);
    }

    if (con->frame_in.allocated)
    {
        os_free(con->frame_in.buf, con->frame_sz);
    }

    if (con->allocated)
    {
        os_free(con, sizeof(iocConnection));
    }
    osal_trace("connection: released");
}


/**
****************************************************************************************************

  @brief Start or prepare the connection.
  @anchor ioc_connect

  The ioc_connect() function...

  @param   con Pointer to the connection object.
  @param   parameters Depending on connection type, can be
           "127.0.0.1:8817" for TCP socket.
  @oaram   newsocket If socket connection is accepted by listening end point, this is
           the socket handle. Otherwise this argument needs to be OS_NULL.
  @param   frame_out_buf Pointer to static frame buffer. OS_NULL to allocate the frame buffer.
  @param   frame_out_buf_sz Size of static frame buffer, either IOC_SOCKET_FRAME_SZ or
           IOC_SERIAL_FRAME_SZ
  @param   frame_in_buf Pointer to static frame buffer. OS_NULL to allocate the frame buffer.
  @param   frame_in_buf_sz Size of static frame buffer, either IOC_SOCKET_FRAME_SZ or
           IOC_SERIAL_FRAME_SZ
  @param   flags Bit fields:
           - IOC_SOCKET Connect with TCP socket.
           - IOC_CREATE_THREAD Create thread to run connection
             (multithread support needed).

  @return  OSAL_SUCCESS if successfull. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus ioc_connect(
    iocConnection *con,
    os_char *parameters,
    osalStream newsocket,
    os_uchar *frame_out_buf,
    int frame_out_buf_sz,
    os_uchar *frame_in_buf,
    int frame_in_buf_sz,
    int flags)
{
    iocRoot
        *root;

    osal_debug_assert(con->debug_id == 'C');

#if OSAL_MULTITHREAD_SUPPORT==0
    /* If we have no multithread support, make sure that
       IOC_CREATE_THREAD flag is not given.
     */
    osal_debug_assert((flags & IOC_CREATE_THREAD) == 0);
#endif

    root = con->link.root;
    ioc_lock(root);
    con->flags = flags;
    con->frame_sz = ((flags & IOC_SOCKET) ? IOC_SOCKET_FRAME_SZ : IOC_SERIAL_FRAME_SZ);

#if OSAL_DEBUG
    if (os_strlen(parameters) > IOC_CONNECTION_PRMSTR_SZ)
    {
        osal_debug_error("Too long parameter string");
    }

    if (frame_out_buf)
    {
        osal_debug_assert(frame_out_buf_sz == con->frame_sz);
    }
#endif
    os_strncpy(con->parameters, parameters, IOC_CONNECTION_PRMSTR_SZ);

    /* Setup or allocate outgoing frame buffer.
     */
    if (frame_out_buf == OS_NULL)
    {
        frame_out_buf = (os_uchar*)os_malloc(con->frame_sz, OS_NULL);
        con->frame_out.allocated = OS_TRUE;
    }
    os_memclear(frame_out_buf, con->frame_sz);
    con->frame_out.buf = frame_out_buf;

    /* Setup or allocate outgoing frame buffer.
     */
    if (frame_in_buf == OS_NULL)
    {
        frame_in_buf = (os_uchar*)os_malloc(con->frame_sz, OS_NULL);
        con->frame_in.allocated = OS_TRUE;
    }
    os_memclear(frame_in_buf, con->frame_sz);
    con->frame_in.buf = frame_in_buf;

#if OSAL_MULTITHREAD_SUPPORT
    /* If we are already running end point thread, stop it. Wait until it has stopped.
     */
    while (ioc_terminate_connection_thread(con))
    {
        ioc_unlock(root);
        os_sleep(50);
        ioc_lock(root);
    }
#endif

    /* If this is incoming TCP socket accepted by end point?
     */
    if (newsocket)
    {
        con->stream = newsocket;
        con->flags |= IOC_CLOSE_CONNECTION_ON_ERROR|IOC_SERVER;

        /* Reset connection state
         */
        ioc_reset_connection_state(con);
    }

#if OSAL_MULTITHREAD_SUPPORT
    /* If we want to run end point in separate thread.
     */
    if (flags & IOC_CREATE_THREAD)
    {
        con->worker.trig = osal_event_create();
        con->worker.thread_running = OS_TRUE;
        con->worker.stop_thread = OS_FALSE;
        osal_thread_create(ioc_connection_thread, con,
            OSAL_THREAD_DETACHED, 0, "connection");
    }
#endif

    ioc_unlock(root);
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Connect and move data.
  @anchor ioc_run_connection

  The ioc_run_connection() function is connects and moves data trough TCP socket or serial
  communication link. It is called repeatedly by ioc_run() and should not be called from
  application.

  @param   con Pointer to the connection object.
  @return  OSAL_SUCCESS if all is running fine. Other return values indicate that the
           connection has broken.

****************************************************************************************************
*/
osalStatus ioc_run_connection(
    iocConnection *con)
{
    osalStatus
        status;

    os_timer
        tnow;

    osal_debug_assert(con->debug_id == 'C');

    /* Do nothing if ioc_connect() has not been called.
     */
    if (con->parameters[0] == '\0')
        return OSAL_SUCCESS;

    /* If stream is not open, then connect it now. Do not try if if two secons have not
       passed since last failed open try.
     */
    if (con->stream == OS_NULL)
    {
        status = ioc_try_to_connect(con);
        if (status == OSAL_STATUS_PENDING) return OSAL_SUCCESS;
        if (status) return status;

        /* Reset connection state
         */
        ioc_reset_connection_state(con);
return OSAL_SUCCESS;
    }

    os_get_timer(&tnow);
    while (OS_TRUE)
    {
        /* Receive as much data as we can
         */
        while (OS_TRUE)
        {
            status = ioc_connection_receive(con);
            if (status == OSAL_STATUS_PENDING)
            {
                break;
            }
            if (status)
            {
                goto failed;
            }

            /* Record timer of last successfull receive.
             */
            con->last_receive = tnow;
        }

        /* Send one frame to connection
         */
        status = ioc_connection_send(con);
        if (status == OSAL_STATUS_PENDING)
        {
            break;
        }
        if (status)
        {
            goto failed;
        }

        /* Record timer of last successfull send.
         */
        con->last_send = tnow;
    }

    /* If too much time elapsed sice last receive
     */
    if (os_elapsed2(&con->last_receive, &tnow, 1000000))
    {
        goto failed;
    }

    /* If time to send keep alive.
     */
    if (os_elapsed2(&con->last_send, &tnow, 100000))
    {
        if (ioc_send_keepalive(con)) goto failed;
        osal_trace("connection: keep alive sent");

        con->last_send = tnow;
    }

    /* Flush data to the connection.
     */
    /* if (con->stream)
    {
        osal_stream_flush(con->stream, 0);
        osal_trace3("stream flushed");
    } */

    return OSAL_SUCCESS;

failed:
    if (con->stream)
    {
        osal_debug_error("stream closed on error");
        osal_stream_close(con->stream);
        con->stream = OS_NULL;
    }
    return OSAL_STATUS_FAILED;
}


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Request to terminate connection worker thread.
  @anchor ioc_terminate_connection_thread

  The ioc_terminate_connection_thread() function sets request to terminate worker thread, if
  one is running the end point.

  ioc_lock() must be on when this function is called.

  @param   con Pointer to the connection object.
  @return  OSAL_SUCCESS if no worker thread is running. OSAL_STATUS_PENDING if worker
           thread is still running.

****************************************************************************************************
*/
osalStatus ioc_terminate_connection_thread(
    iocConnection *con)
{
    osalStatus
        status = OSAL_SUCCESS;

    if (con->worker.thread_running)
    {
        con->worker.stop_thread = OS_TRUE;
        if (con->worker.trig) osal_event_set(con->worker.trig);
        status = OSAL_STATUS_PENDING;
    }

    return status;
}
#endif


/**
****************************************************************************************************

  @brief Try to connect the stream.
  @anchor ioc_try_to_connect

  The ioc_try_to_connect() function tries to open listening TCP socket. Do not try if
  f two secons have not passed since last failed open try.

  @param   con Pointer to the connection object.
  @return  OSAL_SUCCESS if we have succesfully opened the stream. Othervalues indicate
           failure  or delay.

****************************************************************************************************
*/
static osalStatus ioc_try_to_connect(
    iocConnection *con)
{
    osalStatus
        status;

    osalStreamInterface
        *iface;

    os_int
        flags;

    /* If two seconds have not passed since last failed try.
     */
    if (!osal_int64_is_zero(&con->socket_open_fail_timer))
    {
        if (!os_elapsed(&con->socket_open_fail_timer, 2000)) return OSAL_STATUS_PENDING;
    }

    /* Select serial or socket interface by flags and operating system abstraction layer support.
     */
#if OSAL_SERIAL_SUPPORT
    iface = OSAL_SERIAL_IFACE;
#if OSAL_SOCKET_SUPPORT
    iface = (con->flags & IOC_SOCKET) ? OSAL_SOCKET_IFACE : OSAL_SERIAL_IFACE;
#endif
#else
    iface = OSAL_SOCKET_IFACE;
#endif

    /* Try to open listening socket port.
     */
    osal_trace3("connection: opening stream...");
    flags = OSAL_STREAM_CONNECT;
    if (con->flags & IOC_DISABLE_SELECT) flags |= OSAL_STREAM_NO_SELECT;
    con->stream = osal_stream_open(iface, con->parameters, OS_NULL, &status, flags);
    if (con->stream == OS_NULL)
    {
        osal_debug_error("Opening stream failed");
        os_get_timer(&con->socket_open_fail_timer);
        return status;
    }

    /* Success.
     */
    osal_int64_set_zero(&con->socket_open_fail_timer);
    osal_trace("connection: stream opened");
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Reset connection state to start from beginning
  @anchor ioc_reset_connection_state

  The ioc_reset_connection_state() function resets connection state and connected
  source and destination buffers.

  @param   con Pointer to the connection object.
  @return  None.

****************************************************************************************************
*/
static void ioc_reset_connection_state(
    iocConnection *con)
{
    os_timer tnow;
    iocSourceBuffer *sbuf;
    iocTargetBuffer *tbuf;

    con->frame_in.frame_count = 0;
    con->frame_in.pos = 0;
    con->frame_out.frame_count = 0;
    con->frame_out.pos = 0;
    con->frame_out.used = OS_FALSE;

    /* Initialize timers.
     */
    os_get_timer(&tnow);
    con->last_receive = tnow;
    con->last_send = tnow;

    for (sbuf = con->sbuf.first;
         sbuf;
         sbuf = sbuf->clink.next)
    {
        sbuf->changed.range_set = OS_FALSE;
        sbuf->syncbuf.used = OS_FALSE;
        sbuf->syncbuf.make_keyframe = OS_TRUE;
        sbuf->syncbuf.is_keyframe = OS_TRUE;
        sbuf->syncbuf.start_addr = sbuf->syncbuf.end_addr = 0;
    }

    for (tbuf = con->tbuf.first;
         tbuf;
         tbuf = tbuf->clink.next)
    {
        tbuf->is_linked = OS_FALSE;
        tbuf->syncbuf.buf_start_addr = tbuf->syncbuf.buf_end_addr = 0;
        tbuf->syncbuf.buf_used = OS_FALSE;
        tbuf->syncbuf.has_new_data = OS_FALSE;
        tbuf->syncbuf.newdata_start_addr = tbuf->syncbuf.newdata_end_addr = 0;
    }
}


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Connection worker thread function.
  @anchor ioc_connection_thread

  The ioc_connection_thread() function is worker thread to connect a socket (optional) and
  transfer data trough it.

  @param   prm Pointer to parameters for new thread, pointer to end point object.
  @param   done Event to set when parameters have been copied to entry point
           functions own memory.

  @return  None.

****************************************************************************************************
*/
static void ioc_connection_thread(
    void *prm,
    osalEvent done)
{
    iocRoot
        *root;

    iocConnection
        *con;

    osalStatus
        status;

    osalSelectData
        selectdata;

    osal_trace("connection: worker thread started");

    /* Parameters point to the connection object.
     */
    con = (iocConnection*)prm;

    /* Let thread which created this one proceed.
     */
    osal_event_set(done);

    /* Run the end point.
     */
    while (!con->worker.stop_thread)
    {
        /* If stream is not open, then connect it now. Do not try if if two secons have not
           passed since last failed open try.
         */
        if (con->stream == OS_NULL)
        {
            status = ioc_try_to_connect(con);
            if (status == OSAL_STATUS_PENDING) 
            {
                os_sleep(100);
            }
            if (status) 
            {
                osal_debug_error("stream connect try failed");
                goto failed;
            }

            /* Reset connection state
             */
            ioc_reset_connection_state(con);
        }

        status = osal_stream_select(&con->stream, 1, con->worker.trig,
            &selectdata, OSAL_STREAM_DEFAULT);

        if (selectdata.eventflags & OSAL_STREAM_CUSTOM_EVENT)
        {
            osal_trace("socket custom event\n");
        }

        if (selectdata.eventflags & OSAL_STREAM_ACCEPT_EVENT)
        {
            osal_trace("stream accept event");
        }

        if (selectdata.eventflags & OSAL_STREAM_CLOSE_EVENT)
        {
            osal_trace("stream close event");
            break;
        }

        if (selectdata.eventflags & OSAL_STREAM_CONNECT_EVENT)
        {
            osal_trace("stream connect event");

            if (selectdata.errorcode)
            {
                osal_debug_error("stream connect failed");
                break;
            }
        }

        if (selectdata.eventflags & OSAL_STREAM_READ_EVENT)
        {
    //        osal_console_write("read event\n");

            while (OS_TRUE) 
            {
                status = ioc_connection_receive(con);
                if (status == OSAL_STATUS_PENDING)
                {
                    break;
                }
                if (status)
                {
                    goto failed;
                }

                /* Record timer of last successfull receive.
                 */
                os_get_timer(&con->last_receive);
            }
        }

        if (selectdata.eventflags & OSAL_STREAM_WRITE_EVENT)
        {
            osal_console_write("write event\n"); 

            /* Send data to connection
             */
            while (OS_TRUE) 
            {
                status = ioc_connection_send(con);
                if (status == OSAL_STATUS_PENDING)
                {
                    break;
                }
                if (status)
                {
                    goto failed;
                }
                os_get_timer(&con->last_send);
            }
        }

#if 0
        /* If too much time elapsed sice last receive
         */
        if (os_elapsed2(&con->last_receive, &tnow, 1000000))
        {
            goto failed;
        }

        /* If time to send keep alive.
         */
        if (os_elapsed2(&con->last_send, &tnow, 100000))
        {
            if (ioc_send_keepalive(con)) goto failed;
            osal_trace("connection: keep alive sent");

            con->last_send = tnow;
        }

        /* Flush data to the connection.
         */
        /* if (con->stream)
        {
            osal_stream_flush(con->stream, 0);
            osal_trace3("stream flushed");
        } */
#endif

        continue;

failed:
        if (con->stream)
        {
            osal_debug_error("stream closed");
            osal_stream_close(con->stream);
            con->stream = OS_NULL;
        }

        if (con->flags & IOC_CLOSE_CONNECTION_ON_ERROR) break;
    }


    /* Delete trigger event and mark that this thread is no longer running.
     */
    root = con->link.root;
    ioc_lock(root);
    osal_event_delete(con->worker.trig);
    con->worker.trig = OS_NULL;
    con->worker.thread_running = OS_FALSE;

    if (con->flags & IOC_CLOSE_CONNECTION_ON_ERROR)
    {
        ioc_release_connection(con);
    }
    ioc_unlock(root);

    osal_trace("connection: worker thread exited");
}
#endif

