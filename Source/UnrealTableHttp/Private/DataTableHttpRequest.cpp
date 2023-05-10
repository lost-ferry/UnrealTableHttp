#include "DataTableHttpRequest.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void UDataTableHttpRequest::Send(const FString& RequestContent)
{
	if (const UDataTableHttpRequestData* RequestData = BuildRequest())
	{
		FHttpModule& HttpModule = FHttpModule::Get();
		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> NewRequest = HttpModule.CreateRequest();
		NewRequest->SetVerb(RequestData->Verb);
		NewRequest->SetURL(RequestData->Uri);
		NewRequest->SetContentAsString(RequestContent);
		NewRequest->SetTimeout(RequestData->TimeoutSeconds);
		
		for (auto const& Pair : RequestData->Headers)
		{
			NewRequest->SetHeader(Pair.Key, Pair.Value);
		}
		
		NewRequest->OnProcessRequestComplete().BindLambda(
			[&](FHttpRequestPtr& Request, const FHttpResponsePtr& Response, const bool bConnectedSuccessfully) mutable
			{
				const FHttpRequestRow* RequestRecord = RequestRow.GetRow<FHttpRequestRow>("");
				UDataTableHttpResponseHandler* Handler = nullptr;
				if (RequestRecord)
				{
					Handler = NewObject<UDataTableHttpResponseHandler>(this, RequestRecord->ResponseHandler);
				}
				
				if (Response == nullptr)
				{
					if (Handler)
					{
						Handler->HandleFailure(EDataTableHttpRequestStatus::Http_WhyDoYouHateMe, nullptr);
					}

					OnRequestFailed.Broadcast(EDataTableHttpRequestStatus::Http_WhyDoYouHateMe, nullptr);
					return;
				}
				
				UDataTableHttpResponseData* ResponseData = NewObject<UDataTableHttpResponseData>(this);
				EDataTableHttpRequestStatus Status;
				switch(Response->GetResponseCode())
				{
				case 200:
					Status = EDataTableHttpRequestStatus::Http_Ok_200;
				case 201:
					Status = EDataTableHttpRequestStatus::Http_Created_201;
				default:
					Status = EDataTableHttpRequestStatus::Http_WhyDoYouHateMe;
				}
				
				if (bConnectedSuccessfully)
				{
					if (Handler)
					{
						Handler->HandleSuccess(Status, ResponseData);
					}

					OnRequestFailed.Broadcast(Status, ResponseData);
				}
				else
				{
					if (Handler)
					{
						Handler->HandleFailure(Status, ResponseData);
					}

					OnRequestSucceed.Broadcast(Status, ResponseData);
				}
			});

		NewRequest->ProcessRequest();
	}
}

UDataTableHttpRequestData* UDataTableHttpRequest::BuildRequest()
{
	const FHttpRequestRow* RequestRecord = RequestRow.GetRow<FHttpRequestRow>("");
	if (RequestRecord)
	{
		const FHttpServiceRow* ServiceRecord = RequestRecord->Service.GetRow<FHttpServiceRow>("");
		if (ServiceRecord)
		{
			// Merge headers
			TMap<FString, FString> Headers;
			for (const auto& Pair : ServiceRecord->DefaultRequestHeaders)
			{
				Headers.Add(Pair.Key, Pair.Value);
			}

			for (const auto& Pair : RequestRecord->RequestHeaders)
			{
				Headers.Add(Pair.Key, Pair.Value);
			}

			const FString Uri = FString::Printf(TEXT("%s%s"), *ServiceRecord->BaseUrl, *RequestRecord->Path);

			const float Timeout = FMath::Min(ServiceRecord->RequestTimeoutSeconds, RequestRecord->TimeoutSeconds);
			
			UDataTableHttpRequestData* RequestData = NewObject<UDataTableHttpRequestData>(this);
			RequestData->Uri = Uri;
			RequestData->Headers = Headers;
			RequestData->Verb = RequestRecord->VerbToFString();
			RequestData->TimeoutSeconds = Timeout;
			return RequestData;
		}
	}

	return nullptr;
}
