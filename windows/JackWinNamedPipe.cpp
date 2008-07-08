/*
Copyright (C) 2004-2006 Grame

This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackWinNamedPipe.h"
#include "JackError.h"
#include <assert.h>
#include <stdio.h>

#define BUFSIZE 4096

namespace Jack
{

int JackWinNamedPipe::Read(void* data, int len)
{
    DWORD read;
    BOOL res = ReadFile(fNamedPipe, data, len, &read, NULL);
    if (read != (DWORD)len) {
        jack_error("Cannot read named pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinNamedPipe::Write(void* data, int len)
{
    DWORD written;
    BOOL res = WriteFile(fNamedPipe, data, len, &written, NULL);
    if (written != (DWORD)len) {
        jack_error("Cannot write named pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinNamedPipeClient::Connect(const char* dir, int which)
{
    sprintf(fName, "\\\\.\\pipe\\%s_jack_%d", dir, which);
    jack_log("Connect: fName %s", fName);

    fNamedPipe = CreateFile(fName, 			 // pipe name
                            GENERIC_READ |   // read and write access
                            GENERIC_WRITE,
                            0,               // no sharing
                            NULL,            // default security attributes
                            OPEN_EXISTING,   // opens existing pipe
                            0,               // default attributes
                            NULL);          // no template file

    if (fNamedPipe == INVALID_HANDLE_VALUE) {
        jack_error("Cannot connect to named pipe = %s err = %ld", fName, GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinNamedPipeClient::Connect(const char* dir, const char* name, int which)
{
    sprintf(fName, "\\\\.\\pipe\\%s_jack_%s_%d", dir, name, which);
    jack_log("Connect: fName %s", fName);

    fNamedPipe = CreateFile(fName, 			 // pipe name
                            GENERIC_READ |   // read and write access
                            GENERIC_WRITE,
                            0,               // no sharing
                            NULL,            // default security attributes
                            OPEN_EXISTING,   // opens existing pipe
                            0,               // default attributes
                            NULL);          // no template file

    if (fNamedPipe == INVALID_HANDLE_VALUE) {
        jack_error("Cannot connect to named pipe = %s err = %ld", fName, GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinNamedPipeClient::Close()
{
    if (fNamedPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(fNamedPipe);
        fNamedPipe = INVALID_HANDLE_VALUE;
        return 0;
    } else {
        return -1;
    }
}

void JackWinNamedPipeClient::SetReadTimeOut(long sec)
{}

void JackWinNamedPipeClient::SetWriteTimeOut(long sec)
{}

JackWinAsyncNamedPipeClient::JackWinAsyncNamedPipeClient()
        : JackWinNamedPipeClient(), fIOState(kIdle), fPendingIO(false)
{
    fIOState = kIdle;
    fOverlap.hEvent = CreateEvent(NULL,     // default security attribute
                                  TRUE,     // manual-reset event
                                  TRUE,     // initial state = signaled
                                  NULL);   // unnamed event object
}

JackWinAsyncNamedPipeClient::JackWinAsyncNamedPipeClient(HANDLE pipe, bool pending)
        : JackWinNamedPipeClient(pipe), fIOState(kIdle), fPendingIO(pending)
{
    fOverlap.hEvent = CreateEvent(NULL,     // default security attribute
                                  TRUE,     // manual-reset event
                                  TRUE,     // initial state = signaled
                                  NULL);	// unnamed event object

    if (!fPendingIO)
        SetEvent(fOverlap.hEvent);

    fIOState = (fPendingIO) ? kConnecting : kReading;
}

JackWinAsyncNamedPipeClient::~JackWinAsyncNamedPipeClient()
{
    CloseHandle(fOverlap.hEvent);
}

int JackWinAsyncNamedPipeClient::FinishIO()
{
    DWORD success, ret;
    success = GetOverlappedResult(fNamedPipe, 	// handle to pipe
                                  &fOverlap, 	// OVERLAPPED structure
                                  &ret,         // bytes transferred
                                  FALSE);       // do not wait

    switch (fIOState) {

        case kConnecting:
            if (!success) {
                jack_error("Conection error");
                return -1;
            } else {
                fIOState = kReading;
                // Prepare connection for new client ??
            }
            break;

        case kReading:
            if (!success || ret == 0) {
                return -1;
            }
            fIOState = kWriting;
            break;

        case kWriting:
            if (!success || ret == 0) {
                return -1;
            }
            fIOState = kReading;
            break;
    }

    return 0;
}

int JackWinAsyncNamedPipeClient::Read(void* data, int len)
{
    DWORD read;
    jack_log("JackWinNamedPipeClient::Read len = %ld", len);
    BOOL res = ReadFile(fNamedPipe, data, len, &read, &fOverlap);
    jack_log("JackWinNamedPipeClient::Read res = %ld read %ld", res, read);

    if (res && read != 0) {
        fPendingIO = false;
        fIOState = kWriting;
        return 0;
    } else if (!res && GetLastError() == ERROR_IO_PENDING) {
        fPendingIO = true;
        return 0;
    } else {
        jack_error("Cannot read named pipe err = %ld", GetLastError());
        return -1;
    }
}

int JackWinAsyncNamedPipeClient::Write(void* data, int len)
{
    DWORD written;
    jack_log("JackWinNamedPipeClient::Write len = %ld", len);
    BOOL res = WriteFile(fNamedPipe, data, len, &written, &fOverlap);

    if (res && written != 0) {
        fPendingIO = false;
        fIOState = kWriting;
        return 0;
    } else if (!res && GetLastError() == ERROR_IO_PENDING) {
        fPendingIO = true;
        return 0;
    } else {
        jack_error("Cannot write named pipe err = %ld", GetLastError());
        return -1;
    }
}

// Server side

int JackWinNamedPipeServer::Bind(const char* dir, int which)
{
    sprintf(fName, "\\\\.\\pipe\\%s_jack_%d", dir, which);
    jack_log("Bind: fName %s", fName);

    if ((fNamedPipe = CreateNamedPipe(fName,
                                      PIPE_ACCESS_DUPLEX,  // read/write access
                                      PIPE_TYPE_MESSAGE |  // message type pipe
                                      PIPE_READMODE_MESSAGE |  // message-read mode
                                      PIPE_WAIT,  // blocking mode
                                      PIPE_UNLIMITED_INSTANCES,  // max. instances
                                      BUFSIZE,  // output buffer size
                                      BUFSIZE,  // input buffer size
                                      INFINITE,  // client time-out
                                      NULL)) == INVALID_HANDLE_VALUE) { // no security a
        jack_error("Cannot bind server to pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinNamedPipeServer::Bind(const char* dir, const char* name, int which)
{
    sprintf(fName, "\\\\.\\pipe\\%s_jack_%s_%d", dir, name, which);
    jack_log("Bind: fName %s", fName);

    if ((fNamedPipe = CreateNamedPipe(fName,
                                      PIPE_ACCESS_DUPLEX,  // read/write access
                                      PIPE_TYPE_MESSAGE |  // message type pipe
                                      PIPE_READMODE_MESSAGE |  // message-read mode
                                      PIPE_WAIT,  // blocking mode
                                      PIPE_UNLIMITED_INSTANCES,  // max. instances
                                      BUFSIZE,  // output buffer size
                                      BUFSIZE,  // input buffer size
                                      INFINITE,  // client time-out
                                      NULL)) == INVALID_HANDLE_VALUE) { // no security a
        jack_error("Cannot bind server to pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

bool JackWinNamedPipeServer::Accept()
{
    if (ConnectNamedPipe(fNamedPipe, NULL)) {
        return true;
    } else {
        jack_error("Cannot bind server pipe name = %s err = %ld", fName, GetLastError());
        if (GetLastError() == ERROR_PIPE_CONNECTED) {
            jack_error("pipe already connnected = %s ", fName);
            return true;
        } else {
            return false;
        }
    }
}

JackWinNamedPipeClient* JackWinNamedPipeServer::AcceptClient()
{
    if (ConnectNamedPipe(fNamedPipe, NULL)) {
        JackWinNamedPipeClient* client = new JackWinNamedPipeClient(fNamedPipe);
        // Init the pipe to the default value
        fNamedPipe = INVALID_HANDLE_VALUE;
        return client;
    } else {
        switch (GetLastError()) {

            case ERROR_PIPE_CONNECTED:
                return new JackWinNamedPipeClient(fNamedPipe);

            default:
                jack_error("Cannot connect server pipe name = %s  err = %ld", fName, GetLastError());
                return NULL;
                break;
        }
    }
}

int JackWinNamedPipeServer::Close()
{
    jack_log("JackWinNamedPipeServer::Close");

    if (fNamedPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(fNamedPipe);
        CloseHandle(fNamedPipe);
        fNamedPipe = INVALID_HANDLE_VALUE;
        return 0;
    } else {
        return -1;
    }
}

// Server side

int JackWinAsyncNamedPipeServer::Bind(const char* dir, int which)
{
    sprintf(fName, "\\\\.\\pipe\\%s_jack_%d", dir, which);
    jack_log("Bind: fName %s", fName);

    if ((fNamedPipe = CreateNamedPipe(fName,
                                      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // read/write access
                                      PIPE_TYPE_MESSAGE |  // message type pipe
                                      PIPE_READMODE_MESSAGE |  // message-read mode
                                      PIPE_WAIT,  // blocking mode
                                      PIPE_UNLIMITED_INSTANCES,  // max. instances
                                      BUFSIZE,  // output buffer size
                                      BUFSIZE,  // input buffer size
                                      INFINITE,  // client time-out
                                      NULL)) == INVALID_HANDLE_VALUE) { // no security a
        jack_error("Cannot bind server to pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinAsyncNamedPipeServer::Bind(const char* dir, const char* name, int which)
{
    sprintf(fName, "\\\\.\\pipe\\%s_jack_%s_%d", dir, name, which);
    jack_log("Bind: fName %s", fName);

    if ((fNamedPipe = CreateNamedPipe(fName,
                                      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // read/write access
                                      PIPE_TYPE_MESSAGE |  // message type pipe
                                      PIPE_READMODE_MESSAGE |  // message-read mode
                                      PIPE_WAIT,  // blocking mode
                                      PIPE_UNLIMITED_INSTANCES,  // max. instances
                                      BUFSIZE,  // output buffer size
                                      BUFSIZE,  // input buffer size
                                      INFINITE,  // client time-out
                                      NULL)) == INVALID_HANDLE_VALUE) { // no security a
        jack_error("Cannot bind server to pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

bool JackWinAsyncNamedPipeServer::Accept()
{
    return false;
}

JackWinNamedPipeClient* JackWinAsyncNamedPipeServer::AcceptClient()
{
    if (ConnectNamedPipe(fNamedPipe, NULL)) {
        return new JackWinAsyncNamedPipeClient(fNamedPipe, false);
    } else {
        switch (GetLastError()) {

            case ERROR_IO_PENDING:
                return new JackWinAsyncNamedPipeClient(fNamedPipe, true);

            case ERROR_PIPE_CONNECTED:
                return new JackWinAsyncNamedPipeClient(fNamedPipe, false);

            default:
                jack_error("Cannot connect server pipe name = %s  err = %ld", fName, GetLastError());
                return NULL;
                break;
        }
    }
}

} // end of namespace

