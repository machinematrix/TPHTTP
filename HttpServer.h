#ifndef __HTTPSERVER__
#define __HTTPSERVER__
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum
	{
		ServerStatus_Running = 1,
		ServerStatus_Stopped
	} ServerStatus;

	typedef enum
	{
		ServerError_Success,
		ServerError_ThreadCreationError,
		ServerError_StartError,
		ServerError_Running,
		ServerError_AllocationFailed,
		ServerError_WSAStartupError, //Windows only
		ServerError_Initialization,
		ServerError_GetAddrInfo,
		ServerError_Bind,
	} ServerError;

	typedef enum
	{
		ResponseError_Success,
		ResponseError_AllocationFailed,
		ResponseError_InvalidStatusCode,
		ResponseError_InvalidHttpVersion,
		ResponseError_WriteFailed,
	} ResponseError;

	typedef enum
	{
		HttpRequestField_AIM,
		HttpRequestField_Accept,
		HttpRequestField_AcceptCharset,
		HttpRequestField_AcceptDatetime,
		HttpRequestField_AcceptEncoding,
		HttpRequestField_AcceptLanguage,
		HttpRequestField_AccessControlRequestMethod,
		HttpRequestField_AccessControlRequestHeaders,
		HttpRequestField_Authorization,
		HttpRequestField_CacheControl,
		HttpRequestField_Connection,
		HttpRequestField_ContentLength,
		HttpRequestField_ContentMD5,
		HttpRequestField_ContentType,
		HttpRequestField_Cookie,
		HttpRequestField_Date,
		HttpRequestField_Expect,
		HttpRequestField_Forwarded,
		HttpRequestField_From,
		HttpRequestField_Host,
		HttpRequestField_HTTP2Settings,
		HttpRequestField_IfMatch,
		HttpRequestField_IfModifiedSince,
		HttpRequestField_IfNoneMatch,
		HttpRequestField_IfRange,
		HttpRequestField_IfUnmodifiedSince,
		HttpRequestField_MaxForwards,
		HttpRequestField_Origin,
		HttpRequestField_Pragma,
		HttpRequestField_ProxyAuthorization,
		HttpRequestField_Range,
		HttpRequestField_Referer,
		HttpRequestField_TE,
		HttpRequestField_Trailer,
		HttpRequestField_TransferEncoding,
		HttpRequestField_UserAgent,
		HttpRequestField_Upgrade,
		HttpRequestField_Via,
		HttpRequestField_Warning, //last field
	} HttpRequestField; //https://en.wikipedia.org/wiki/List_of_HTTP_header_fields#Standard_request_fields

	typedef enum
	{
		HttpResponseField_AccessControlAllowOrigin,
		HttpResponseField_AccessControlAllowCredentials,
		HttpResponseField_AccessControlExposeHeaders,
		HttpResponseField_AccessControlMaxAge,
		HttpResponseField_AccessControlAllowMethods,
		HttpResponseField_AccessControlAllowHeaders,
		HttpResponseField_AcceptPatch,
		HttpResponseField_AcceptRanges,
		HttpResponseField_Age,
		HttpResponseField_Allow,
		HttpResponseField_AltSvc,
		HttpResponseField_CacheControl,
		HttpResponseField_Connection,
		HttpResponseField_ContentDisposition,
		HttpResponseField_ContentEncoding,
		HttpResponseField_ContentLanguage,
		HttpResponseField_ContentLength,
		HttpResponseField_ContentLocation,
		HttpResponseField_ContentMD5,
		HttpResponseField_ContentRange,
		HttpResponseField_ContentType, //https://developer.mozilla.org/es/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Lista_completa_de_tipos_MIME
		HttpResponseField_Date,
		HttpResponseField_DeltaBase,
		HttpResponseField_ETag,
		HttpResponseField_Expires,
		HttpResponseField_IM,
		HttpResponseField_LastModified,
		HttpResponseField_Link,
		HttpResponseField_Location,
		HttpResponseField_P3P,
		HttpResponseField_Pragma,
		HttpResponseField_ProxyAuthenticate,
		HttpResponseField_PublicKeyPins,
		HttpResponseField_RetryAfter,
		HttpResponseField_Server,
		HttpResponseField_SetCookie,
		HttpResponseField_StrictTransportSecurity,
		HttpResponseField_Trailer,
		HttpResponseField_TransferEncoding,
		HttpResponseField_Tk,
		HttpResponseField_Upgrade,
		HttpResponseField_Vary,
		HttpResponseField_Via,
		HttpResponseField_Warning,
		HttpResponseField_WWWAuthenticate,
		HttpResponseField_XFrameOptions
	} HttpResponseField; //https://en.wikipedia.org/wiki/List_of_HTTP_header_fields#Standard_response_fields

	typedef struct HttpServer *HttpServerHandle;
	typedef struct HttpRequest *HttpRequestHandle;
	typedef struct HttpResponse *HttpResponseHandle;

	typedef void(HandlerCallback)(HttpRequestHandle);
	typedef void(LoggerCallback)(const char*);

	HttpServerHandle HttpServer_Create(unsigned short port, ServerError *errorCode);
	int HttpServer_Start(HttpServerHandle server);
	void HttpServer_Destroy(HttpServerHandle server);
	int HttpServer_SetEndpointCallback(HttpServerHandle server, const char *resource, HandlerCallback *callback);
	ServerStatus HttpServer_GetStatus(HttpServerHandle server);
	const char* HttpServer_GetServerError(ServerError errorCode);
	ServerError HttpServer_GetErrorCode(HttpServerHandle server);
	int HttpServer_SetLoggerCallback(HttpServerHandle server, LoggerCallback *callback);

	HttpResponseHandle HttpServer_CreateResponse(HttpRequestHandle request);
	void HttpServer_DestroyResponse(HttpResponseHandle response);
	int HttpServer_SetResponseStatusCode(HttpResponseHandle response, unsigned short statusCode);
	int HttpServer_SetResponseField(HttpResponseHandle response, HttpResponseField field, const char *value);
	int HttpServer_SetResponseBody(HttpResponseHandle response, const void *body, size_t bodyLength);
	int HttpServer_SendResponse(HttpResponseHandle response);
	const char* HttpServer_GetResponseError(HttpResponseHandle response);

	const char* HttpServer_GetRequestMethod(HttpRequestHandle request);
	const char* HttpServer_GetRequestUri(HttpRequestHandle request);
	const char* HttpServer_GetRequestVersion(HttpRequestHandle request);
	const char* HttpServer_GetRequestField(HttpRequestHandle request, HttpRequestField field);
	const void* HttpServer_GetRequestBody(HttpRequestHandle request);
	unsigned long long HttpServer_GetRequestBodySize(HttpRequestHandle request);

#ifdef __cplusplus
}
#endif

#endif