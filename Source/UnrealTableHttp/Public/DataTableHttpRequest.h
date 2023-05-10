#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Engine/DataTable.h"
#include "DataTableHttpRequest.generated.h"

UENUM(BlueprintType)
enum class EHttpVerb : uint8
{
	Get     UMETA(DisplayName="GET"),
	Put     UMETA(DisplayName="PUT"),
	Post    UMETA(DisplayName="POST"),
	Patch   UMETA(DisplayName="PATCH"),
	Delete  UMETA(DisplayName="DELETE"),
};

UENUM(BlueprintType)
enum class EDataTableHttpRequestStatus : uint8
{
	Http_Ok_200 UMETA(DisplayName="HTTP_OK_200"),
	Http_Created_201 UMETA(DisplayName="HTTP_CREATED_201"),
	Http_NoContent_204 UMETA(DisplayName="HTTP_NO_CONTENT_204"),
	Http_MovedPermanently_301 UMETA(DisplayName="HTTP_MOVED_PERMANENTLY_301"),
	Http_BadRequest_400 UMETA(DisplayName="HTTP_MOVED_PERMANENTLY_400"),
	Http_Unauthorized_401 UMETA(DisplayName="HTTP_UNAUTORIZED_401"),
	Http_PaymentRequired_402 UMETA(DisplayName="HTTP_PAYMENT_REQUIRED_402"),
	Http_Forbidden_403 UMETA(DisplayName="HTTP_FORBIDDEN_403"),
	Http_NotFound_404 UMETA(DisplayName="HTTP_NOT_FOUND_403"),
	Http_MethodNotAllowed_405 UMETA(DisplayName="HTTP_METHOD_NOT_ALLOWED_405"),
	Http_RequestTimeout_408 UMETA(DisplayName="HTTP_REQUEST_TIMEOUT_408"),
	Http_TooManyRequests_429 UMETA(DisplayName="HTTP_TOO_MANY_REQUESTS_429"),
	Http_InternalServerError_500 UMETA(DisplayName="HTTP_INTERNAL_SERVER_ERROR_500"),
	Http_NotImplemented_501 UMETA(DisplayName="HTTP_NOT_IMPLEMENTED_501"),
	Http_BadGateway_502 UMETA(DisplayName="HTTP_BAD_GATEWAY_502"),
	Http_ServiceUnavailable_503 UMETA(DisplayName="HTTP_SERVICE_UNAVAILABLE_503"),
	Http_GatewayTimeout_504 UMETA(DisplayName="HTTP_GATEWAY_TIMEOUT_504"),
	Http_WhyDoYouHateMe UMETA(DisplayName="HTTP_WHY_DO_YOU_HATE_ME"),
};

UCLASS()
class UDataTableHttpRequestData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	FString Verb;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	FString Uri;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	TMap<FString, FString> Headers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	float TimeoutSeconds;
};

UCLASS()
class UDataTableHttpResponseData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	TMap<FString, FString> Headers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	FString ResponseContent;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDataTableHttpRequestFailDelegate, EDataTableHttpRequestStatus, Status, UDataTableHttpResponseData*, ResponseData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDataTableHttpRequestSuccessDelegate, EDataTableHttpRequestStatus, Status, UDataTableHttpResponseData*, ResponseData);

UCLASS(Blueprintable, BlueprintType)
class UDataTableHttpResponseHandler: public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry", meta = (RowType = "HttpRequestRow"))
	FDataTableRowHandle RequestRow;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LostFerry")
	void HandleSuccess(const EDataTableHttpRequestStatus& Status, const UDataTableHttpResponseData* Response);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LostFerry")
	void HandleFailure(const EDataTableHttpRequestStatus& Status, const UDataTableHttpResponseData* Response);

	virtual void HandleSuccess_Implementation(const EDataTableHttpRequestStatus& Status, const UDataTableHttpResponseData* Response) {};

	virtual void HandleFailure_Implementation(const EDataTableHttpRequestStatus& Status, const UDataTableHttpResponseData* Response) {};
};

USTRUCT(BlueprintType)
struct FHttpServiceRow : public FTableRowBase
{
	GENERATED_BODY()
	
	/** This should be same as the RowName value of the DataTable row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	FString RecordName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	FString BaseUrl;

	/** These can be overwritten in the specific request. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	TMap<FString, FString> DefaultRequestHeaders;

	/** Will select lower of the service or request timeout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	int RequestTimeoutSeconds { 5 };
};

USTRUCT(BlueprintType)
struct FHttpRequestRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	FString RecordName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry", meta = (RowType = "HttpServiceRow"))
	FDataTableRowHandle Service;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	EHttpVerb Verb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	FString Path;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	TMap<FString, FString> RequestHeaders;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	int TimeoutSeconds { 5 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry")
	TSubclassOf<UDataTableHttpResponseHandler> ResponseHandler;

	/**
	 * @brief Return appropriate FString for the selected HttpVerb.
	 * @return Defaults to "GET" if no match.
	 */
	FString VerbToFString() const
	{
		switch(Verb)
		{
		case EHttpVerb::Get:
			return TEXT("GET");
		case EHttpVerb::Put:
			return TEXT("PUT");
		case EHttpVerb::Post:
			return TEXT("POST");
		case EHttpVerb::Patch:
			return TEXT("PATCH");
		case EHttpVerb::Delete:
			return TEXT("DELETE");
		default:
			return TEXT("GET");
		}
	}
};

UCLASS(Blueprintable, BlueprintType)
class UDataTableHttpRequest : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LostFerry", meta = (RowType = "HttpRequestRow"))
	FDataTableRowHandle RequestRow;
	
	UFUNCTION(BlueprintCallable, Category="LostFerry")
	void Send(const FString& RequestContent);

	UFUNCTION(BlueprintCallable, Category="LostFerry")
	UDataTableHttpRequestData* BuildRequest();

	UPROPERTY(BlueprintAssignable, Category="LostFerry")
	FDataTableHttpRequestFailDelegate OnRequestFailed;

	UPROPERTY(BlueprintAssignable, Category="LostFerry")
	FDataTableHttpRequestSuccessDelegate OnRequestSucceed;
};
