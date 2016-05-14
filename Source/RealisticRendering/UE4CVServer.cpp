// Fill out your copyright notice in the Description page of Project Settings.

#include "RealisticRendering.h"
#include "UE4CVServer.h"

FUE4CVServer::FUE4CVServer(FCommandDispatcher* CommandDispatcher)
{
	this->CommandDispatcher = CommandDispatcher;
	// Command Dispatcher defines the behavior of the server, TODO: Write with javadoc style

	this->NetworkManager = NewObject<UNetworkManager>();
	this->NetworkManager->AddToRoot(); // Avoid GC

	this->NetworkManager->OnReceived().AddRaw(this, &FUE4CVServer::HandleRequest);
	this->NetworkManager->OnReceived().AddRaw(this, &FUE4CVServer::LogClientMessage);
}

FUE4CVServer::~FUE4CVServer()
{
	this->NetworkManager->FinishDestroy(); // TODO: Check is this usage correct?
}

bool FUE4CVServer::Start()
{
	NetworkManager->Start();
	return true;
}

void FUE4CVServer::LogClientMessage(const FString& RawMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *RawMessage);
}

void FUE4CVServer::HandleRequest(const FString& RawMessage)
{
	// Parse Raw Message
	FString MessageFormat = "(\\d{1,8}):(.*)";
	FRegexPattern RegexPattern(MessageFormat);
	FRegexMatcher Matcher(RegexPattern, RawMessage);

	if (Matcher.FindNext())
	{
		// TODO: Handle malform request message
		FString StrRequestId = Matcher.GetCaptureGroup(1);
		FString Message = Matcher.GetCaptureGroup(2);

		FExecStatus ExecStatus = CommandDispatcher->Exec(Message);
		uint32 RequestId = FCString::Atoi(*StrRequestId);

		ReplyClient(RequestId, ExecStatus);
	}
	else
	{
		SendClientMessage(FString::Printf(TEXT("error: Malformat raw message '%s'"), *RawMessage));
	}
}

void FUE4CVServer::SendClientMessage(FString Message)
{
	NetworkManager->SendMessage(Message);
}

void FUE4CVServer::ReplyClient(uint32 RequestId, FExecStatus ExecStatus)
{
	FString Message = FString::Printf(TEXT("%d:%s"), RequestId, *ExecStatus.Message);
	NetworkManager->SendMessage(Message);
}
