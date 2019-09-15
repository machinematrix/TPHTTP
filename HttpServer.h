#ifndef __HTTPSERVER__
#define __HTTPSERVER__

enum State
{
    Running = 1,
	Stopped
};

enum HttpResponseField
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
	HttpResponseField_ContentType,
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
void HttpServer_SetEndpointCallback(HttpServerHandle server, const char *resource, HandlerCallback callback);
int HttpServer_GetStatus(HttpServerHandle server);

HttpResponseHandle HttpServer_CreateResponse();
void HttpServer_DestroyResponse(HttpResponseHandle response);
void HttpServer_SetResponseField(HttpResponseHandle response, int field, const char *value);
void HttpServer_SetResponseBody(HttpResponseHandle response, const void *body, unsigned long long bodyLength);

void HttpServer_SendHtml(HttpRequestHandle request, const char *html);
void HttpServer_SendResponse(HttpRequestHandle request, HttpResponseHandle response);
char* HttpServer_GetRequestUri(HttpRequestHandle request);
const char* HttpServer_GetErrorMessage(HttpServerHandle server);

#endif