//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _FILEREQUESTHANDLER_H_
#define _FILEREQUESTHANDLER_H_


#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"
#include "core/stream/fileStream.h"

// Implementation of the factory for creating file load handlers.
class ClientFileHandlerFactory : public CefSchemeHandlerFactory {
public:
   ClientFileHandlerFactory() {}

   // Return a new scheme handler instance to handle the request.
   CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      const CefString& scheme_name,
      CefRefPtr<CefRequest> request) OVERRIDE;

private:
   IMPLEMENT_REFCOUNTING(ClientFileHandlerFactory);
   DISALLOW_COPY_AND_ASSIGN(ClientFileHandlerFactory);
};

// Implementation of the resource handler for file:// and t3d:// requests.
class FileStream;
class FileRequestHandler : public CefResourceHandler {
public:
   FileRequestHandler() : _offset(0), _data_size(0) {}
   virtual ~FileRequestHandler();

   virtual bool Open(CefRefPtr<CefRequest> request,
      bool& handle_request,
      CefRefPtr<CefCallback> callback) OVERRIDE;

   virtual void GetResponseHeaders(CefRefPtr<CefResponse> response,
      int64& response_length,
      CefString& redirectUrl) OVERRIDE;

   virtual bool Skip(int64 bytes_to_skip,
      int64& bytes_skipped,
      CefRefPtr<CefResourceSkipCallback> callback) OVERRIDE;

   virtual bool Read(void* data_out,
      int bytes_to_read,
      int& bytes_read,
      CefRefPtr<CefResourceReadCallback> callback) OVERRIDE;

   virtual void Cancel() OVERRIDE;

   virtual void tryOpenFile();

private:
   bool uriToFilePath(const CefString& uri);
   FileStream _file_stream;
   CefString _file_uri;
   CefRefPtr<CefCallback> _callback_ptr;
   Torque::Path _file_path;
   String _mime_type;

   size_t _offset;
   size_t _data_size;

   IMPLEMENT_REFCOUNTING(FileRequestHandler);
   DISALLOW_COPY_AND_ASSIGN(FileRequestHandler);
};

#endif  // _FILEREQUESTHANDLER_H_
