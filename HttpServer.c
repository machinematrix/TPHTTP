#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __linux__
#define _GNU_SOURCE
#endif

#include "HttpServer.h"
#include "Thread.h"
#include "Vector.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#define Sleep(miliseconds) usleep((miliseconds) * 1000)
typedef int Socket;
#endif

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <Ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
typedef SOCKET Socket;
typedef SSIZE_T ssize_t;
#define close closesocket
#endif

enum MakeHttpRequestResults {
	MakeHttpRequestResult_BadRequest,
	MakeHttpRequestResult_AllocationFailed,
	MakeHttpRequestResult_Success,
};

typedef struct
{
	char *context;
	HandlerCallback *callback;
} HandlerSlot;

struct HttpServer
{
    Socket sock;
    int status;
    int queueLength;
	char strPort[6];
    Mutex mtx;
    Thread serverThread;
	VectorHandle handlerVector;
	LoggerCallback *logger;
	int errorCode;
};

struct HttpRequest //Usada para pasarle el request a las callbacks de los endpoints
{
	Socket sock;
	
	char *method;
	char *resource;
	char *version;
	char *fields[HttpRequestField_Warning + 1];
	char *body;
	size_t bodySize;
};

struct HttpResponse //Usada para mandarle datos al cliente
{
	int errorCode;
	Socket sock;
	char *version;
	char *statusCode;
	char *fields[HttpResponseField_XFrameOptions + 1];
	char *body;
	size_t bodySize;
};

typedef struct //Usada para pasarle informacion a httpParser
{
	Socket sock;
	HttpServerHandle server;
} HttpRequestInfo;

static size_t integerDigitCount(size_t num)
{
	size_t result = 0;

	while (num > 0) {
		++result;
		num /= 10;
	}

	return result;
}

static char *reverseCharSearch(char *haystack, char sought)
{
	int i;
	char *result = NULL;

	for(i = (int)strlen(haystack) - 1; i > -1 && !result; --i)
	{
		if (haystack[i] == sought) {
			result = haystack + i;
		}
	}

	return result;
}

static const char *getHttpResponseFieldText(int field)
{
	switch (field)
	{
	case HttpResponseField_AccessControlAllowOrigin:
		return "Access-Control-Allow-Origin: ";
	case HttpResponseField_AccessControlAllowCredentials:
		return "Access-Control-Allow-Credentials: ";
	case HttpResponseField_AccessControlExposeHeaders:
		return "Access-Control-Expose-Headers: ";
	case HttpResponseField_AccessControlMaxAge:
		return "Access-Control-Max-Age: ";
	case HttpResponseField_AccessControlAllowMethods:
		return "Access-Control-Allow-Methods: ";
	case HttpResponseField_AccessControlAllowHeaders:
		return "Access-Control-Allow-Headers: ";
	case HttpResponseField_AcceptPatch:
		return "Accept-Patch: ";
	case HttpResponseField_AcceptRanges:
		return "Accept-Ranges: ";
	case HttpResponseField_Age:
		return "Age: ";
	case HttpResponseField_Allow:
		return "Allow: ";
	case HttpResponseField_AltSvc:
		return "Alt-Svc: ";
	case HttpResponseField_CacheControl:
		return "Cache-Control: ";
	case HttpResponseField_Connection:
		return "Connection: ";
	case HttpResponseField_ContentDisposition:
		return "Content-Disposition: ";
	case HttpResponseField_ContentEncoding:
		return "Content-Encoding: ";
	case HttpResponseField_ContentLanguage:
		return "Content-Language: ";
	case HttpResponseField_ContentLength:
		return "Content-Length: ";
	case HttpResponseField_ContentLocation:
		return "Content-Location: ";
	case HttpResponseField_ContentMD5:
		return "Content-MD5: ";
	case HttpResponseField_ContentRange:
		return "Content-Range: ";
	case HttpResponseField_ContentType:
		return "Content-Type: ";
	case HttpResponseField_Date:
		return "Date: ";
	case HttpResponseField_DeltaBase:
		return "Delta-Base: ";
	case HttpResponseField_ETag:
		return "ETag: ";
	case HttpResponseField_Expires:
		return "Expires: ";
	case HttpResponseField_IM:
		return "IM: ";
	case HttpResponseField_LastModified:
		return "Last-Modified: ";
	case HttpResponseField_Link:
		return "Link: ";
	case HttpResponseField_Location:
		return "Location: ";
	case HttpResponseField_P3P:
		return "P3P: ";
	case HttpResponseField_Pragma:
		return "Pragma: ";
	case HttpResponseField_ProxyAuthenticate:
		return "Proxy-Authenticate: ";
	case HttpResponseField_PublicKeyPins:
		return "Public-Key-Pins: ";
	case HttpResponseField_RetryAfter:
		return "Retry-After: ";
	case HttpResponseField_Server:
		return "Server: ";
	case HttpResponseField_SetCookie:
		return "Set-Cookie: ";
	case HttpResponseField_StrictTransportSecurity:
		return "Strict-Transport-Security: ";
	case HttpResponseField_Trailer:
		return "Trailer: ";
	case HttpResponseField_TransferEncoding:
		return "Transfer-Encoding: ";
	case HttpResponseField_Tk:
		return "Tk: ";
	case HttpResponseField_Upgrade:
		return "Upgrade: ";
	case HttpResponseField_Vary:
		return "Vary: ";
	case HttpResponseField_Via:
		return "Via: ";
	case HttpResponseField_Warning:
		return "Warning: ";
	case HttpResponseField_WWWAuthenticate:
		return "WWW-Authenticate: ";
	case HttpResponseField_XFrameOptions:
		return "X-Frame-Options: ";
	default:
		return NULL;
	}
}

static const char *getHttpRequestFieldText(size_t field)
{
	switch (field)
	{
	case HttpRequestField_AIM:
		return "A-IM";
	case HttpRequestField_Accept:
		return "Accept";
	case HttpRequestField_AcceptCharset:
		return "Accept-Charset";
	case HttpRequestField_AcceptDatetime:
		return "Accept-Datetime";
	case HttpRequestField_AcceptEncoding:
		return "Accept-Encoding";
	case HttpRequestField_AcceptLanguage:
		return "Accept-Language";
	case HttpRequestField_AccessControlRequestMethod:
		return "Access-Control-Request-Method";
	case HttpRequestField_AccessControlRequestHeaders:
		return "Access-Control-Request-Method";
	case HttpRequestField_Authorization:
		return "Authorization";
	case HttpRequestField_CacheControl:
		return "Cache-Control";
	case HttpRequestField_Connection:
		return "Connection";
	case HttpRequestField_ContentLength:
		return "Content-Length";
	case HttpRequestField_ContentMD5:
		return "Content-MD5";
	case HttpRequestField_ContentType:
		return "Content-Type";
	case HttpRequestField_Cookie:
		return "Content-Cookie";
	case HttpRequestField_Date:
		return "Date";
	case HttpRequestField_Expect:
		return "Expect";
	case HttpRequestField_Forwarded:
		return "Forwarded";
	case HttpRequestField_From:
		return "From";
	case HttpRequestField_Host:
		return "Host";
	case HttpRequestField_HTTP2Settings:
		return "HTTP2-Settings";
	case HttpRequestField_IfMatch:
		return "If-Match";
	case HttpRequestField_IfModifiedSince:
		return "If-Modified-Since";
	case HttpRequestField_IfNoneMatch:
		return "If-None-Match";
	case HttpRequestField_IfRange:
		return "If-Range";
	case HttpRequestField_IfUnmodifiedSince:
		return "If-Unmodified-Since";
	case HttpRequestField_MaxForwards:
		return "Max-Forwards";
	case HttpRequestField_Origin:
		return "Origin";
	case HttpRequestField_Pragma:
		return "Pragma";
	case HttpRequestField_ProxyAuthorization:
		return "Proxy-Authorization";
	case HttpRequestField_Range:
		return "Range";
	case HttpRequestField_Referer:
		return "Referer";
	case HttpRequestField_TE:
		return "TE";
	case HttpRequestField_Trailer:
		return "Trailer";
	case HttpRequestField_TransferEncoding:
		return "Transfer-Encoding";
	case HttpRequestField_UserAgent:
		return "User-Agent";
	case HttpRequestField_Upgrade:
		return "Upgrade";
	case HttpRequestField_Via:
		return "Via";
	case HttpRequestField_Warning:
		return "Warning";
	default:
		return NULL;
	}
}

static void HttpServerSetStatus(HttpServerHandle sv, int state)
{
	lockMutex(sv->mtx);
	sv->status = state;
	unlockMutex(sv->mtx);
}

static int myWrite(Socket sock, const void *buffer, size_t size)
{
	int result = 0;
	size_t bytesSent = 0;

	while (bytesSent < size)
	{
		ssize_t tempBytesSent = send(sock, (char*)buffer + bytesSent, (int)(size - bytesSent), 0);

		if (tempBytesSent > 0) {
			bytesSent += tempBytesSent;
		}
		else {
			#ifdef __linux__
			result = errno;
			#endif
			#ifdef _WIN32
			result = WSAGetLastError();
			#endif
			break;
		}
	}

	return result;
}

static char* getHttpRequestText(Socket sock)
{
	int flags, chances = 10;
	size_t capacity = 512, chunkSize = capacity / 2, offset = 0;
	ssize_t bytesRead;
	char *result = malloc(capacity);
	
	#ifdef _WIN32
	flags = 0;
	ioctlsocket(sock, FIONBIO, &(u_long) { 1 });
	#endif
	#ifdef __linux__
	flags = MSG_DONTWAIT;
	#endif

	if (result)
	{
		do
		{
			if (offset >= capacity) {
				capacity *= 2;
				void *aux = realloc(result, capacity);
				if (aux)
					result = aux;
			}
			
			bytesRead = recv(sock, result + offset, (int)chunkSize, flags);

			if (bytesRead != SOCKET_ERROR && bytesRead > 0) {
				offset += (size_t)bytesRead;
				chances = 10; //resetear chances
			}
			else {
				Sleep(25);
				--chances;
			}
		}
		while (chances);

		if (offset) { //si leimos al menos un byte
			void *aux = realloc(result, offset + 1); //space for null terminator
			if (aux) {
				result = aux;
				result[offset] = 0; //set null terminator
			}
		}
		else {
			free(result);
			result = NULL;
		}
	}

	#ifdef _WIN32
	ioctlsocket(sock, FIONBIO, &(u_long) { 0 });
	#endif

	return result;
}

static int MakeHttpRequest(HttpRequestInfo *info, struct HttpRequest *result)
{
	size_t i;
	const char *methodEnd, *resourceBegin, *resourceEnd, *versionBegin, *versionEnd, *bodyBegin, *bodyEnd, *versionPrefix = "HTTP/";
	char *requestText = getHttpRequestText(info->sock);
	
	if (!requestText)
		return MakeHttpRequestResult_BadRequest;

	//Get delimiters
	methodEnd = strchr(requestText, ' ');
	if (!methodEnd)
		return MakeHttpRequestResult_BadRequest;
	resourceBegin = strchr(methodEnd, '/');
	if (!resourceBegin)
		return MakeHttpRequestResult_BadRequest;
	resourceEnd = strchr(resourceBegin, ' ');
	if (!resourceEnd)
		return MakeHttpRequestResult_BadRequest;
	versionBegin = strstr(resourceEnd, versionPrefix); //search for string "HTTP/"
	if (!versionBegin)
		return MakeHttpRequestResult_BadRequest;
	versionBegin += strlen(versionPrefix);
	versionEnd = strstr(versionBegin, "\r\n");
	if (!versionEnd)
		return MakeHttpRequestResult_BadRequest;
	bodyBegin = strstr(versionEnd, "\r\n\r\n");
	if (!bodyBegin)
		return MakeHttpRequestResult_BadRequest;
	bodyBegin += 4;
	bodyEnd = bodyBegin + strlen(bodyBegin);

	memset(result, 0, sizeof(struct HttpRequest));

	result->method = malloc((size_t)(methodEnd - requestText + 1));
	if (!result->method)
		goto AllocationError;
	strncpy(result->method, requestText, (size_t)(methodEnd - requestText));
	result->method[methodEnd - requestText] = 0;

	result->resource = malloc((size_t)(resourceEnd - resourceBegin + 1));
	if (!result->resource)
		goto AllocationError;
	strncpy(result->resource, resourceBegin, (size_t)(resourceEnd - resourceBegin));
	result->resource[resourceEnd - resourceBegin] = 0;

	result->version = malloc((size_t)(versionEnd - versionBegin + 1));
	if (!result->version)
		goto AllocationError;
	strncpy(result->version, versionBegin, (size_t)(versionEnd - versionBegin));
	result->version[versionEnd - versionBegin] = 0;

	//Get fields
	for (i = HttpRequestField_AIM; i < HttpRequestField_Warning + 1; ++i)
	{
		const char *field = getHttpRequestFieldText(i);
		char *fieldName = strstr(requestText, field);

		if (fieldName)
		{
			size_t fieldNameSize = strlen(field) + 2;
			if (fieldName[fieldNameSize - 1] != ' ') //Si no hay un espacio despues del :, decrementar
				--fieldNameSize;

			char *fieldValueBegin = fieldName + fieldNameSize, *fieldValueEnd = strstr(fieldValueBegin, "\r\n");
			result->fields[i] = malloc((size_t)(fieldValueEnd - fieldValueBegin + 1));

			if (result->fields[i]) {
				strncpy(result->fields[i], fieldValueBegin, (size_t)(fieldValueEnd - fieldValueBegin));
				result->fields[i][fieldValueEnd - fieldValueBegin] = 0;
			}
			else
				goto AllocationError;
		}
	}

	if (bodyEnd - bodyBegin)
	{
		result->bodySize = (size_t)(bodyEnd - bodyBegin);
		result->body = malloc(result->bodySize);
		if (result->body) {
			strcpy(result->body, bodyBegin);
		}
		else
			goto AllocationError;
	}

	free(requestText);
	result->sock = info->sock;

	return MakeHttpRequestResult_Success;

AllocationError:
	free(result->method);
	free(result->resource);
	free(result->version);
	for (i = HttpRequestField_AIM; i < HttpRequestField_Warning + 1; ++i)
		free(result->fields[i]);
	free(result->body);
	free(requestText);
	return MakeHttpRequestResult_AllocationFailed;
}

static void destroyRequest(HttpRequestHandle request)
{
	size_t i;
	free(request->version);
	free(request->resource);

	for (i = HttpRequestField_AIM; i < HttpRequestField_Warning + 1; ++i)
		free(request->fields[i]);

	free(request->body);
}

static void TryLog(HttpServerHandle server, const char *msg)
{
	if (server->logger)
		server->logger(msg);
}

static void *dispatcher(void *arg)
{
	HttpRequestInfo *info = arg;

	size_t handlerIndex = ~0u;
	size_t i, sz, bestMatchLength = 0;
	struct HttpRequest req;
	int makeRequestResult = MakeHttpRequest(info, &req);

	if (makeRequestResult == MakeHttpRequestResult_Success)
	{
		for (i = 0, sz = Vector_Size(info->server->handlerVector); i < sz; ++i)
		{
			HandlerSlot *slot = Vector_At(info->server->handlerVector, i);
			char *lastSlash = reverseCharSearch(req.resource, '/'), *match = strstr(req.resource, slot->context);
			int matchedUntilLastSlash = !strncmp(slot->context, req.resource, (size_t)(lastSlash - req.resource));

			if (match && match == req.resource && matchedUntilLastSlash)
			{
				size_t contextLength = strlen(slot->context);

				if (contextLength > bestMatchLength) {
					handlerIndex = i;
					bestMatchLength = contextLength;
				}
			}
		}

		if (handlerIndex != ~0u) {
			((HandlerSlot *)Vector_At(info->server->handlerVector, handlerIndex))->callback(&req);
			TryLog(info->server, "Served request");
		}
		else {
			const char *notFoundResponse = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
			myWrite(info->sock, notFoundResponse, strlen(notFoundResponse));
		}
		destroyRequest(&req);
	}
	else
	{
		if (makeRequestResult == MakeHttpRequestResult_BadRequest)
		{
			const char *badRequest = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
			myWrite(info->sock, badRequest, strlen(badRequest));
			TryLog(info->server, "Bad request");
		}
		else if (makeRequestResult == MakeHttpRequestResult_AllocationFailed) {
			TryLog(info->server, "Allocation failed while assembling request");
		}
	}

	close(info->sock);
	free(info);
	return NULL;
}

static void* serverProcedure(void *arg)
{
    HttpServerHandle svData = arg;
    
	if (!listen(svData->sock, svData->queueLength))
	{
		#if defined(USE_SELECT) && !defined(USE_POLL)
		Socket i;
		fd_set descriptors;
		Socket maxFd = svData->sock;
		FD_ZERO(&descriptors);
		FD_SET(svData->sock, &descriptors);
		#endif

		while (HttpServer_GetStatus(svData) == ServerStatus_Running)
		{
			#if defined(USE_SELECT) && !defined(USE_POLL)
			if (select(maxFd + 1, &descriptors, NULL, NULL, NULL) != -1)
			{
				for (i = 0; i <= maxFd; ++i)
				{
					if (FD_ISSET(i, &descriptors))
					{
						if (i == svData->sock)
						{
							Socket newFd = accept(i, NULL, NULL);
							if (newFd != SOCKET_ERROR)
							{
								if (newFd > maxFd)
									maxFd = newFd;
								FD_SET(newFd, &descriptors);
							}
						}
						else
						{
							HttpRequestInfo *info = calloc(1, sizeof(HttpRequestInfo));
							if (info) {
								info->sock = i;
								info->server = svData;
								dispatcher(info);
								FD_CLR(i, &descriptors);
							}
						}
					}
				}
			}
			#else
			Socket clientSocket = accept(svData->sock, NULL, NULL);
			if (clientSocket != SOCKET_ERROR)
			{
				HttpRequestInfo *info = calloc(1, sizeof(HttpRequestInfo));
				if (info)
				{
					info->sock = clientSocket;
					info->server = svData;

					Thread th = createThread(dispatcher, info);
					if (th)
						destroyThread(th);
				}
			}
			#endif
		}
	}
	else {
		HttpServerSetStatus(svData, ServerStatus_Stopped);
		svData->errorCode = ServerError_StartError;
		return NULL;
	}

	HttpServerSetStatus(svData, ServerStatus_Stopped);
    return NULL;
}

static char poke(HttpServerHandle data) //returns 1 if it could poke server, 0 otherwise
{
	char result = 0;
	Socket socketHandle = socket(AF_INET, SOCK_STREAM, 0);

	if (socketHandle != INVALID_SOCKET)
	{
		struct addrinfo *list = NULL, hint = { 0 };

		hint.ai_family = AF_INET;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_flags = AI_NUMERICSERV;

		if (!getaddrinfo(NULL, data->strPort, &hint, &list))
		{
			if (!connect(socketHandle, list->ai_addr, sizeof(struct addrinfo))) {
				const char *req = "lol";
				if(!myWrite(socketHandle, req, strlen(req)))
					result = 1;
			}

			freeaddrinfo(list);
		}

		close(socketHandle);
	}
	return result;
}

static void createServerErrorHandler(HttpServerHandle *sv)
{
	free(*sv);
	*sv = NULL;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//API---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

HttpServerHandle HttpServer_Create(unsigned short port)
{
	#ifdef _WIN32
	WSADATA wsaData = { 0 };
	if (!WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		#endif

		HttpServerHandle sv = calloc(1, sizeof(struct HttpServer));

		if (sv)
		{
			sv->queueLength = 5;
			sv->sock = socket(AF_INET, SOCK_STREAM, 0);
			sv->status = ServerStatus_Stopped;
			sv->mtx = createMutex();
			sv->handlerVector = Vector_Create(sizeof(HandlerSlot));
			char optval[8] = { 1 };

			if (sv->mtx
				&& sv->handlerVector
				&& sv->sock != INVALID_SOCKET
				&& !setsockopt(sv->sock, SOL_SOCKET, SO_REUSEADDR, optval, sizeof(optval)))
			{
				struct addrinfo *list = NULL, hint = { 0 };

				sprintf(sv->strPort, "%d", port);
				hint.ai_flags = AI_NUMERICSERV | AI_PASSIVE;
				hint.ai_family = AF_INET; //IPv4
				hint.ai_socktype = SOCK_STREAM;
				hint.ai_protocol = IPPROTO_TCP;

				if (!getaddrinfo(NULL, sv->strPort, &hint, &list))
				{
					if (bind(sv->sock, list->ai_addr, (int)list->ai_addrlen) == SOCKET_ERROR)
					{
						close(sv->sock);
						createServerErrorHandler(&sv);
					}
					freeaddrinfo(list);
				}
				else {
					createServerErrorHandler(&sv);
				}
			}
			else {
				createServerErrorHandler(&sv);
			}
		}
		return sv;
	#ifdef _WIN32
	}
	return NULL;
	#endif
}

int HttpServer_Start(HttpServerHandle server)
{
	int result = 1;

	server->errorCode = ServerError_Success;
	HttpServerSetStatus(server, ServerStatus_Running);
	server->serverThread = createThread(serverProcedure, server);
	
	if (!server->serverThread) {
		result = 0;
		HttpServerSetStatus(server, ServerStatus_Stopped);
		server->errorCode = ServerError_ThreadCreationError;
	}

	return result;
}

void HttpServer_Destroy(HttpServerHandle server)
{
	if (server)
	{
		size_t i, sz;
		if (HttpServer_GetStatus(server) == ServerStatus_Running) {
			HttpServerSetStatus(server, ServerStatus_Stopped); //To break the loop
			poke(server);
		}
		
		if (server->serverThread) { //si trato de destruir un server que no fue arrancado, el thread es nulo.
			joinThread(server->serverThread);
			destroyThread(server->serverThread);
		}
		close(server->sock);
		destroyMutex(server->mtx);

		for (i = 0, sz = Vector_Size(server->handlerVector); i < sz; ++i) {
			HandlerSlot *slot = Vector_At(server->handlerVector, i);
			free(slot->context);
		}

		Vector_Destroy(server->handlerVector);
		free(server);

		#ifdef _WIN32
		WSACleanup();
		#endif
	}
}

int HttpServer_SetEndpointCallback(HttpServerHandle server, const char *resource, HandlerCallback *callback)
{
	int result = 1;

	server->errorCode = ServerError_Success;
	if (HttpServer_GetStatus(server) != ServerStatus_Running)
	{
		size_t i, sz;
		HandlerSlot slot = { 0 };

		for (i = 0, sz = Vector_Size(server->handlerVector); i < sz; ++i)
		{
			HandlerSlot *auxSlot = Vector_At(server->handlerVector, i);
			if (!strcmp(resource, auxSlot->context)) { //Si ya hay un slot para ese resource, sobreescribir su callback con la nueva
				auxSlot->callback = callback;
				return result;
			}
		}

		slot.callback = callback;
		slot.context = calloc(1, strlen(resource) + 1);

		if(slot.context) {
			strcpy(slot.context, resource);
			Vector_PushBack(server->handlerVector, &slot);
		}
		else {
			Vector_PopBack(server->handlerVector);
			server->errorCode = ServerError_AllocationFailed;
			result = 0;
		}
	}
	else {
		server->errorCode = ServerError_Running;
		result = 0;
	}

	return result;
}

int HttpServer_GetStatus(HttpServerHandle server)
{
	lockMutex(server->mtx);
	int result = server->status;
	unlockMutex(server->mtx);
	return result;
}

const char *HttpServer_GetServerError(HttpServerHandle server)
{
	switch (server->errorCode)
	{
	case ServerError_Success:
		return "Success";
	case ServerError_ThreadCreationError:
		return "Could not create server thread";
	case ServerError_StartError:
		return "Could not start server";
	case ServerError_Running:
		return "Can't perform operation while server is running";
	default:
		return "";
	}
}

int HttpServer_SetLoggerCallback(HttpServerHandle server, LoggerCallback *callback)
{
	int result = 1;
	if (HttpServer_GetStatus(server) != ServerStatus_Running) {
		server->logger = callback;
	}
	else {
		server->errorCode = ServerError_Running;
		result = 0;
	}
	return result;
}

HttpResponseHandle HttpServer_CreateResponse(HttpRequestHandle request)
{
	HttpResponseHandle result = calloc(1, sizeof(struct HttpResponse));
	
	if (result)
	{
		const char *version = "1.1";
		size_t versionLength = strlen(version);
		result->sock = request->sock;
		result->version = malloc(versionLength + 1);
		if (result->version)
		{
			strcpy(result->version, version);
			result->version[versionLength] = 0;
		}
	}

	return result;
}

void HttpServer_DestroyResponse(HttpResponseHandle response)
{
	if (response)
	{
		size_t i;
		free(response->version);
		free(response->statusCode);

		for (i = 0; i < HttpResponseField_XFrameOptions + 1; ++i)
			free(response->fields[i]);

		free(response->body);
		free(response);
	}
}

int HttpServer_SetResponseStatusCode(HttpResponseHandle response, short statusCode)
{
	int result = 1;
	response->errorCode = ResponseError_Success;
	size_t codeLength = integerDigitCount((size_t)statusCode) + 1;

	if (statusCode <= 599) {
		if ((response->statusCode = malloc(codeLength))) {
			sprintf(response->statusCode, "%d", statusCode);
		}
		else {
			response->errorCode = ResponseError_AllocationFailed;
			result = 0;
		}
	}
	else {
		response->errorCode = ResponseError_InvalidStatusCode;
		result = 0;
	}

	return result;
}

int HttpServer_SetResponseField(HttpResponseHandle response, int field, const char *value)
{
	int result = 0;
	void *fieldTemp = malloc(strlen(value) + 1);

	response->errorCode = ResponseError_Success;
	if (fieldTemp) {
		free(response->fields[field]);
		response->fields[field] = fieldTemp;
		strcpy(response->fields[field], value);
		result = 1;
	}
	else
		response->errorCode = ResponseError_AllocationFailed;

	return result;
}

int HttpServer_SetResponseBody(HttpResponseHandle response, const void *body, size_t bodyLength)
{
	int result = 0;
	void *bodyTemp = malloc(bodyLength);

	response->errorCode = ResponseError_Success;
	if (bodyTemp)
	{
		free(response->body);
		response->body = bodyTemp;
		response->bodySize = bodyLength;
		memcpy(response->body, body, bodyLength);
		result = 1;
	}
	else
		response->errorCode = ResponseError_AllocationFailed;

	return result;
}

int HttpServer_SendResponse(HttpResponseHandle response)
{
	if (!response->version) {
		response->errorCode = ResponseError_InvalidHttpVersion;
		return 0;
	}
	if (!response->statusCode) {
		response->errorCode = ResponseError_InvalidStatusCode;
		return 0;
	}

	static const char *responseBegin = "HTTP/", *fieldEnd = "\r\n";
	size_t i;

	VectorHandle resp = Vector_Create(sizeof(char));
	if (!resp)
		goto AllocationError;
	Vector_InsertRange(resp, Vector_Size(resp), responseBegin, responseBegin + strlen(responseBegin)); //header begin
	if (Vector_AllocFailed(resp))
		goto AllocationError;
	Vector_InsertRange(resp, Vector_Size(resp), response->version, response->version + strlen(response->version)); //version
	if (Vector_AllocFailed(resp))
		goto AllocationError;
	Vector_PushBack(resp, &(char){ ' ' }); //space
	if (Vector_AllocFailed(resp))
		goto AllocationError;
	Vector_InsertRange(resp, Vector_Size(resp), response->statusCode, response->statusCode + strlen(response->statusCode));
	if (Vector_AllocFailed(resp))
		goto AllocationError;
	Vector_InsertRange(resp, Vector_Size(resp), fieldEnd, fieldEnd + strlen(fieldEnd)); //new line
	if (Vector_AllocFailed(resp))
		goto AllocationError;

	for (i = 0; i < HttpResponseField_XFrameOptions + 1; ++i)
	{
		if (response->fields[i]) {
			const char *fieldName = getHttpResponseFieldText((int)i);
			Vector_InsertRange(resp, Vector_Size(resp), fieldName, fieldName + strlen(fieldName)); //field name
			if (Vector_AllocFailed(resp))
				goto AllocationError;
			Vector_InsertRange(resp, Vector_Size(resp), response->fields[i], response->fields[i] + strlen(response->fields[i])); // field value
			if (Vector_AllocFailed(resp))
				goto AllocationError;
			Vector_InsertRange(resp, Vector_Size(resp), fieldEnd, fieldEnd + strlen(fieldEnd)); // new line
			if (Vector_AllocFailed(resp))
				goto AllocationError;
		}
	}
	
	Vector_InsertRange(resp, Vector_Size(resp), fieldEnd, fieldEnd + strlen(fieldEnd)); //header end
	if (Vector_AllocFailed(resp))
		goto AllocationError;

	if (response->bodySize) {
		Vector_InsertRange(resp, Vector_Size(resp), response->body, response->body + response->bodySize); //body
		if (Vector_AllocFailed(resp))
			goto AllocationError;
	}

	if (myWrite(response->sock, Vector_At(resp, 0), Vector_Size(resp) * sizeof(char))) {
		response->errorCode = ResponseError_WriteFailed;
		Vector_Destroy(resp);
		return 0;
	}

	Vector_Destroy(resp);
	return 1;

AllocationError:
	response->errorCode = ResponseError_AllocationFailed;
	Vector_Destroy(resp);
	return 0;
}

const char *HttpServer_GetResponseError(HttpResponseHandle response)
{
	switch (response->errorCode)
	{
	case ResponseError_Success:
		return "Success";
	case ResponseError_AllocationFailed:
		return "Memory allocation failed, download more RAM";
	case ResponseError_InvalidStatusCode:
		return "Invalid status code in response";
	case ResponseError_InvalidHttpVersion:
		return "Invalid HTTP version in response";
	case ResponseError_WriteFailed:
		return "Write failed";
	default:
		return "";
	}
}

const char *HttpServer_GetRequestMethod(HttpRequestHandle request)
{
	return request->method;
}

const char* HttpServer_GetRequestUri(HttpRequestHandle request)
{
	return request->resource;
}

const char* HttpServer_GetRequestVersion(HttpRequestHandle request)
{
	return request->version;
}

const char* HttpServer_GetRequestField(HttpRequestHandle request, int field)
{
	if (field <= HttpRequestField_Warning) {
		return request->fields[field];
	}
	else return NULL;
}

const void* HttpServer_GetRequestBody(HttpRequestHandle request)
{
	return request->body;
}

unsigned long long HttpServer_GetRequestBodySize(HttpRequestHandle request)
{
	return request->bodySize;
}