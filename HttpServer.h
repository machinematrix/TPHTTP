#ifndef __HTTPSERVER__
#define __HTTPSERVER__

enum State
{
    Running = 1,
	Stopped
};

enum ServerError
{
	ServerError_Success,
	ServerError_ThreadCreationError,
	ServerError_StartError,
	ServerError_Running,
	ServerError_AllocationFailed,
};

enum ResponseError
{
	ResponseError_Success,
	ResponseError_AllocationFailed,
	ResponseError_InvalidStatusCode,
	ResponseError_InvalidHttpVersion,
	ResponseError_WriteFailed,
};

enum HttpResponseField //https://en.wikipedia.org/wiki/List_of_HTTP_header_fields#Standard_response_fields
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
};

typedef struct HttpServer *HttpServerHandle;
typedef struct HttpRequest *HttpRequestHandle;
typedef struct HttpResponse *HttpResponseHandle;
typedef void(*HandlerCallback)(HttpRequestHandle);

HttpServerHandle HttpServer_Create();
void HttpServer_Start(HttpServerHandle server);
void HttpServer_Destroy(HttpServerHandle server);
int HttpServer_SetEndpointCallback(HttpServerHandle server, const char *resource, HandlerCallback callback);
int HttpServer_GetStatus(HttpServerHandle server);
const char* HttpServer_GetErrorMessage(HttpServerHandle server);

HttpResponseHandle HttpServer_CreateResponse(HttpRequestHandle request);
void HttpServer_DestroyResponse(HttpResponseHandle response);
int HttpServer_SetResponseStatusCode(HttpResponseHandle response, short statusCode);
int HttpServer_SetResponseField(HttpResponseHandle response, int field, const char *value);
int HttpServer_SetResponseBody(HttpResponseHandle response, const void *body, unsigned long long bodyLength);
int HttpServer_SendResponse(HttpResponseHandle response);
const char* HttpServer_GetResponseError(HttpResponseHandle response);

void HttpServer_SendHtml(HttpRequestHandle request, const char *html);
char* HttpServer_GetRequestUri(HttpRequestHandle request);

#endif