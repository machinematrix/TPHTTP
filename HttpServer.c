#ifdef __linux__
#define _GNU_SOURCE
#endif

#include "HttpServer.h"
#include "Thread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

typedef int Socket;
#endif

#ifdef _WIN32
//#include <Ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#define ioctl ioctlsocket
#define errno (WSAGetLastError())
typedef SOCKET Socket;
#endif

typedef struct
{
	char *context;
	HandlerCallback callback;
} HandlerSlot;

struct HttpServer
{
    Socket sock;
    int status;
    int queueLength;
    Mutex mtx;
    Thread serverThread;
	HandlerSlot *handlerVector;
	size_t handlerVectorSize;
	size_t handlerVectorCapacity;
	int errorCode;
};

struct HttpRequest //make function to destroy this
{
	Socket sock;
	//const char *requestText;
	
	char *method;
	char *resource;
	char *version;
	char *fields[HttpRequestField_Warning + 1];
	char *body;
	size_t bodySize;
};

struct HttpResponse
{
	int errorCode;
	Socket sock;
	char *version;
	char *statusCode;
	char *fields[HttpResponseField_XFrameOptions + 1];
	char *body;
	size_t bodySize;
};

typedef struct
{
	Socket sock;
	HttpServerHandle server;
} HttpRequestInfo;

static int integerDigitCount(size_t num)
{
	int result = 0;

	while (num > 0)
	{
		++result;
		num /= 10;
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

static const char *getHttpRequestFieldText(int field)
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

//static char* getHttpRequestHeader(int sock)
//{
//	ssize_t size = 64, offset = 0;
//	size_t chunkSize = 32;
//	char *result = calloc((size_t)size, sizeof(char));
//	char cr = 0, nl = 0, cr2 = 0, nl2 = 0;
//
//	while (!(cr && nl && cr2 && nl2))
//	{	
//		if (offset == size) {
//			size *= 2;
//			result = realloc(result, (size_t)size);
//		}
//		offset += read(sock, result + (size_t)offset, 1);
//		char token = result[offset - 1];
//
//		if (token != '\r' && token != '\n') {
//			cr = nl = cr2 = nl2 = 0;
//		}
//		else
//		{
//			if (token == '\r') {
//				if (!cr)
//					cr = 1;
//				else
//					cr2 = 1;
//			}
//			else if (token == '\n') {
//				if (!nl)
//					nl = 1;
//				else
//					nl2 = 1;
//			}
//		}
//	}
//
//	result = realloc(result, (size_t)offset + 1); //space for null terminator
//	result[offset] = 0; //set null terminator
//
//	return result;
//}

static int myWrite(Socket sock, const void *buffer, size_t size)
{
	int result = 0;

	size_t bytesSent = 0;
	while (bytesSent < size)
	{
		size_t tempBytesSent = (size_t)write(sock, buffer + bytesSent, size - bytesSent);
		if (tempBytesSent > 0) {
			bytesSent += tempBytesSent;
		}
		else {
			result = errno;
			break;
		}
	}

	return result;
}

//resultado es memoria dinamica
static char* getHttpRequestText(int sock)
{
	ssize_t size = 256, offset = 0, bytesRead;
	size_t chunkSize = ((size_t)size) / 2;
	char *result = malloc((size_t)size);

	do
	{
		if (offset >= size) {
			size *= 2;
			result = realloc(result, (size_t)size);
		}

		bytesRead = recv(sock, result + (size_t)offset, chunkSize, MSG_DONTWAIT);

		if (bytesRead > 0) {
			offset += bytesRead;
		}
	} while (bytesRead > 0);

	result = realloc(result, (size_t)offset + 1); //space for null terminator
	result[offset] = 0; //set null terminator

	return result;
}

//resultado es memoria dinamica
static char* getResourceText(char *requestText)
{
	char *beg = strchr(requestText, '/'), *end = (beg ? beg + strcspn(beg, " ?") : NULL), *result = NULL;
	if (beg && end)
	{
		result = malloc((size_t)(end - beg + 1));
		memcpy(result, beg, (size_t)(end - beg));
		result[end - beg] = 0; //null character
	}
	return result;
}

static struct HttpRequest makeRequest(const char *requestText) //Add error handling. this assumes thaat requestText points to a 100% valid HTTP request.
{
	struct HttpRequest result = {};
	size_t i;

	//Get method
	const char *methodEnd = strchr(requestText, ' ');
	result.method = malloc((size_t)(methodEnd - requestText + 1));
	strncpy(result.method, requestText, (size_t)(methodEnd - requestText));
	result.method[methodEnd - requestText] = 0;

	//Get resource
	const char *resourceBegin = methodEnd + 1, *resourceEnd = strchr(resourceBegin, ' ');
	result.resource = malloc((size_t)(resourceEnd - resourceBegin + 1));
	strncpy(result.resource, resourceBegin, (size_t)(resourceEnd - resourceBegin));
	result.resource[resourceEnd - resourceBegin] = 0;

	//Get version
	const char *versionBegin = strchr(resourceEnd, ' ') + 1, *versionEnd = strstr(versionBegin, "\r\n");
	result.version = malloc((size_t)(versionEnd - versionBegin + 1));
	strncpy(result.version, versionBegin, (size_t)(versionEnd - versionBegin));
	result.version[versionEnd - versionBegin] = 0;

	//Get fields
	for (i = HttpRequestField_AIM; i < HttpRequestField_Warning + 1; ++i)
	{
		const char *field = getHttpRequestFieldText((int)i);
		char *fieldName = strstr(requestText, field);

		if (fieldName)
		{
			size_t fieldNameSize = strlen(field) + 2 /*": "*/;
			char *fieldValueBegin = fieldName + fieldNameSize, *fieldValueEnd = strstr(fieldValueBegin, "\r\n");
			
			if ((result.fields[i] = malloc((size_t)(fieldValueEnd - fieldValueBegin + 1)))) {
				strncpy(result.fields[i], fieldValueBegin, (size_t)(fieldValueEnd - fieldValueBegin));
				result.fields[i][fieldValueEnd - fieldValueBegin] = 0;
			}
		}
	}

	//Get body
	const char *bodyBegin = strstr(versionEnd, "\r\n\r\n") + 4, *bodyEnd = bodyBegin + strlen(bodyBegin); //Esto asume que requestText termina con '\0'
	result.bodySize = (size_t)(bodyEnd - bodyBegin);
	result.body = malloc(result.bodySize);
	strcpy(result.body, bodyBegin);

	return result;
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

static void *httpParser(void *arg)
{
	HttpRequestInfo *info = arg;
	char *request = getHttpRequestText(info->sock);

	if (request)
	{
		char *resource = getResourceText(request);

		if (resource)
		{
			size_t i;
			//char handled = 0;
			size_t handlerIndex = -1u, bestMatchLength = 0;

			for (i = 0; i < info->server->handlerVectorSize; ++i)
			{
				/*if (!strcmp(resource, info->server->handlerVector[i].resource)) {
					struct HttpRequest req = { info->sock, request };
					info->server->handlerVector[i].callback(&req);
					handled = 1;
				}*/
				char *match = strstr(resource, info->server->handlerVector[i].context);
				
				if (match && match == resource) //si matcheo al principio del string
				{ //Make a match function that doesn't make a literal compare. Instead, compare everything except slashes
					size_t contextLength = strlen(info->server->handlerVector[i].context);

					if (contextLength > bestMatchLength) {
						handlerIndex = i;
						bestMatchLength = contextLength;
					}
				}
			}

			if (handlerIndex != -1u)
			{
				struct HttpRequest req = makeRequest(request);
				//struct HttpRequest req = { info->sock, request };
				info->server->handlerVector[handlerIndex].callback(&req);
				destroyRequest(&req);
			}
			else {
				const char *notFoundResponse = "HTTP/1.1 404 NOTFOUND\r\nConnection: close\r\n\r\n";
				myWrite(info->sock, notFoundResponse, strlen(notFoundResponse));
			}

			/*if (!handled) {
				const char *notFoundResponse = "HTTP/1.1 404 NOTFOUND\r\nConnection: close\r\n\r\n";
				myWrite(info->sock, notFoundResponse, strlen(notFoundResponse));
			}*/
			free(resource);
		}
		free(request);
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
		while (HttpServer_GetStatus(svData) == Running)
		{
			struct sockaddr data = {};
			socklen_t sockLen = sizeof(struct sockaddr);
			int clientSocket = accept(svData->sock, &data, &sockLen);

			if (clientSocket != -1)
			{
				HttpRequestInfo *info = calloc(1, sizeof(HttpRequestInfo));
				info->sock = clientSocket;
				info->server = svData;
				createThread(httpParser, PTHREAD_CREATE_JOINABLE, info);
			}
		}
	}
	else {
		HttpServerSetStatus(svData, Stopped);
		svData->errorCode = ServerError_StartError;
		return NULL;
	}

	HttpServerSetStatus(svData, Stopped);
    return NULL;
}

static char poke(HttpServerHandle data) //returns 1 if it could poke server, 0 otherwise
{
	char result = 0;
	Socket socketHandle = socket(AF_INET, SOCK_STREAM, 0);

	if (socketHandle != -1)
	{
		struct addrinfo *list = NULL, hint = {};

		hint.ai_family = AF_INET;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;

		if (!getaddrinfo(NULL, "http", &hint, &list))
		{
			if (!connect(socketHandle, list->ai_addr, sizeof(struct addrinfo))) {
				myWrite(socketHandle, "\r\n\r\n", 4);
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
    //printf("%s\n", strerror(errno));
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//API---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

HttpServerHandle HttpServer_Create()
{
    HttpServerHandle sv = calloc(1, sizeof(struct HttpServer));
    
	sv->queueLength = 5;
    sv->sock = socket(AF_INET, SOCK_STREAM, 0);
	sv->status = Stopped;

    if(!createMutex(sv->mtx) && sv->sock != -1 && !setsockopt(sv->sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)))
    {
        int addrInfoResult = 0, bindResult = 0;
        struct addrinfo *list = NULL, hint = {};
        
        hint.ai_flags = AI_NUMERICSERV | AI_PASSIVE;
		hint.ai_family = AF_INET; //IPv4
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		
		if(!(addrInfoResult = getaddrinfo(NULL, "80", &hint, &list)))
		{
		    if((bindResult = bind(sv->sock, list->ai_addr, sizeof(struct addrinfo))) != -1)
		    {
				//HttpServerSetStatus(sv, Stopped);
		    }
		    else {
		        close(sv->sock);
		        createServerErrorHandler(&sv);
		    }
		}
		else {
		    createServerErrorHandler(&sv);
	    }
    }
    else {
        createServerErrorHandler(&sv);
    }
    
    return sv;
}

void HttpServer_Start(HttpServerHandle server)
{
	server->errorCode = ServerError_Success;
	HttpServerSetStatus(server, Running);
	server->serverThread = createThread(serverProcedure, PTHREAD_CREATE_JOINABLE, server); //creacion de thread puede fallar
	
	if (!server->serverThread) {
		HttpServerSetStatus(server, Stopped);
		server->errorCode = ServerError_ThreadCreationError;
	}
}

void HttpServer_Destroy(HttpServerHandle server)
{
	if (server)
	{
		size_t i;
		if (HttpServer_GetStatus(server) == Running) {
			HttpServerSetStatus(server, Stopped); //To break the loop
			poke(server);
		}

		joinThread(server->serverThread);
		assert(!close(server->sock));
		deleteMutex(server->mtx);

		for (i = 0; i < server->handlerVectorSize; ++i) {
			free(server->handlerVector[i].context);
		}
		free(server->handlerVector);

		free(server);
	}
}

int HttpServer_SetEndpointCallback(HttpServerHandle server, const char *resource, HandlerCallback callback)
{
	int result = 1;

	server->errorCode = ServerError_Success;
	if (HttpServer_GetStatus(server) != Running)
	{
		size_t i;
		for (i = 0; i < server->handlerVectorSize; ++i)
		{
			if (!strcmp(resource, server->handlerVector[i].context)) { //Si ya hay un slot para ese resource, sobreescribir su callback con la nueva
				server->handlerVector[i].callback = callback;
				return result;
			}
		}

		if (server->handlerVectorSize == server->handlerVectorCapacity) {
			server->handlerVectorCapacity = (server->handlerVectorCapacity ? server->handlerVectorCapacity * 2 : 1);
			server->handlerVector = realloc(server->handlerVector, server->handlerVectorCapacity * sizeof(HandlerSlot)); //add error handling
		}

		HandlerSlot *slot = server->handlerVector + server->handlerVectorSize++;
		slot->callback = callback;
		slot->context = calloc(1, strlen(resource) + 1);
		strcpy(slot->context, resource);
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
		return "Can't set handler while server is running";
	default:
		return "";
	}
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
		strcpy(result->version, version);
		result->version[versionLength] = 0;
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

	if (statusCode <= 599) {
		if ((response->statusCode = malloc((size_t)(integerDigitCount((size_t)statusCode) + 1)))) {
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

int HttpServer_SetResponseBody(HttpResponseHandle response, const void *body, unsigned long long bodyLength)
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

	char *preparedResponse, responseBegin[] = "HTTP/"; //agregar principio del header en estructura
	size_t i, offset = 0, responseSize = response->bodySize + strlen(responseBegin) + strlen(response->version) + 1 + strlen(response->statusCode) + 2 + 2; //+2 is 2 bytes needed for header end

	for (i = 0; i < HttpResponseField_XFrameOptions + 1; ++i) {
		if (response->fields[i])
			responseSize += strlen(getHttpResponseFieldText((int)i)) + strlen(response->fields[i]) + 2; //+2 is /r/n
	}

	if ((preparedResponse = malloc(responseSize)))
	{
		offset += (size_t)sprintf(preparedResponse + offset, "%s%s %s\r\n", responseBegin, response->version, response->statusCode);

		for (i = 0; i < HttpResponseField_XFrameOptions + 1; ++i) {
			if (response->fields[i])
				offset += (size_t)sprintf(preparedResponse + offset, "%s%s\r\n", getHttpResponseFieldText((int)i), response->fields[i]);
		}
		offset += (size_t)sprintf(preparedResponse + offset, "\r\n"); //fin del header. escribo 2 bytes porque el bucle, o la primera linea, ya ponen /r/n al principio

		if (response->body)
			memcpy(preparedResponse + offset, response->body, response->bodySize); //escribir body

		if (myWrite(response->sock, preparedResponse, responseSize)) {
			response->errorCode = ResponseError_WriteFailed;
			free(preparedResponse);
			return 0;
		}

		free(preparedResponse);
		return 1;
	}
	else {
		response->errorCode = ResponseError_AllocationFailed;
		return 0;
	}
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

void HttpServer_SendHtml(HttpRequestHandle request, const char *html)
{
	const char *responseTemplate1 = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-Length: ", *responseTemplate2 = "\r\nConnection: close\r\n\r\n";
	char strNum[16], *response;
	size_t htmlLen = strlen(html);

	sprintf(strNum, "%d", htmlLen);
	response = malloc(strlen(responseTemplate1) + strlen(strNum) + strlen(responseTemplate2) + htmlLen + 1);
	sprintf(response, "%s%s%s%s", responseTemplate1, strNum, responseTemplate2, html);
	
	myWrite(request->sock, response, strlen(response));
	
	free(response);
}

const char *HttpServer_GetRequestMethod(HttpRequestHandle request)
{
	return request->method;
}

const char* HttpServer_GetRequestUri(HttpRequestHandle request)
{
	return request->resource;
	//char *beg = strchr(request->requestText, '/'), *end = (beg ? beg + strcspn(beg, " ") : NULL), *result = NULL;
	//if (beg && end)
	//{
	//	result = malloc((size_t)(end - beg + 1));
	//	memcpy(result, beg, (size_t)(end - beg));
	//	result[end - beg] = 0; //null character
	//}
	//return result;
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
