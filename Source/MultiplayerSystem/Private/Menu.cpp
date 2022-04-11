// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "Components/Button.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if(World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if(PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	const UGameInstance* GameInstance = GetGameInstance();
	if(GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if(MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &UMenu::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionComplete.AddUObject(this, &UMenu::OnFindSession);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &UMenu::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySession.AddDynamic(this, &UMenu::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSession.AddDynamic(this, &UMenu::OnStartSession);
	}
}

bool UMenu::Initialize()
{
	if(!Super::Initialize())
	{
		return  false;
	}

	if(HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);
	}
	if(JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);
	}

	return true;
}

void UMenu::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);

	MenuTearDown();
}

void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	if(GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Hosting")));
	}

	if(MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	if(MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSession(10000);
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Joining")));
		}
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* World = GetWorld();
	if(World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if(PC)
		{
			FInputModeGameOnly InputGameMode;
			PC->SetInputMode(InputGameMode);
			PC->SetShowMouseCursor(false);
		}
	}
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if(bWasSuccessful)
	{
		UWorld* World = GetWorld();
		if(World)
		{
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccessful)
{
	if(MultiplayerSessionsSubsystem == nullptr) return;

	for(auto Result : SessionResult)
	{
		FString SettingValue;
		Result.Session.SessionSettings.Get("MatchType", SettingValue);
		if(SettingValue == MatchType)
		{
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}

	if(!bWasSuccessful || SessionResult.Num()<=0)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}

	if(Result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
	if(bWasSuccessful)
	{
		UWorld* World = GetWorld();
		if(World)
		{
			World->ServerTravel("/Game/ThirdPersonCPP/Maps/Map?listen");
		}
	}
}
