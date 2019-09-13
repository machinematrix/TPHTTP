#define _GNU_SOURCE
#include <stdio.h>
#include "HttpServer.h"

void hello(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:DodgerBlue;\">Hello World</h1>");
}

void bye(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<h1 style=\"background-color:Tomato;\">Bye World</h1>");
}

void image(HttpRequestPtr req)
{
	HttpServer_SendHtml(req, "<img src=\"https://miro.medium.com/max/4096/1*vs239SecVBaB4HvLsZ8O5Q.png\" alt=\"Italian Trulli\">");
}

int main(void)
{
    HttpServerPtr sv = HttpServer_Create();
	
	if (sv)
	{
		HttpServer_SetEndpointCallback(sv, "/hello", hello);
		HttpServer_SetEndpointCallback(sv, "/bye", bye);
		HttpServer_SetEndpointCallback(sv, "/image", image);
		HttpServer_Start(sv);

		if (HttpServer_GetStatus(sv) == Running)
		{
			int continuar = 1;

			while (continuar)
			{
				scanf("%d", &continuar);
			}
		}
		else
			printf("%s\n", HttpServer_GetError(sv));

		HttpServer_Destroy(sv);
	}
	else printf("Could not create server\n");
    return 0;
}