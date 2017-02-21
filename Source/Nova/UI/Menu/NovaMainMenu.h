﻿// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaMenu.h"
#include "Nova/Tools/NovaActorTools.h"


// Sub-menu types
enum class ENovaMainMenuType : uint8
{
	Home,
	Game,
	Flight,
	Assembly,
	Settings
};

// Main menu class
class SNovaMainMenu : public SNovaMenu
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenu)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	SNovaMainMenu();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
		Interaction
	----------------------------------------------------*/

	/** Show this menu */
	void Show();

	/** Start hiding this menu */
	void Hide();

	/** Start displaying the tooltip */
	void ShowTooltip(SWidget* TargetWidget, FText Content);

	/** Stop displaying the tooltip */
	void HideTooltip(SWidget* TargetWidget);
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;


	/*----------------------------------------------------
		Content callbacks
	----------------------------------------------------*/

protected:

	/** Check if we can display the home menu */
	bool IsHomeMenuVisible() const;

	/** Check if we can display the game menus */
	bool AreGameMenusVisible() const;

	/** Check if we can display the flight menu */
	bool IsFlightMenuVisible() const;

	/** Check if we can display the assembly menu */
	bool IsAssemblyMenuVisible() const;

	/** Get the close button text */
	FText GetCloseText() const;

	/** Get the close button help text */
	FText GetCloseHelpText() const;

	/** Get the title text */
	FText GetTooltipText() const;

	/** Get the tooltip color */
	FSlateColor GetTooltipColor() const;

	/** Get the title text */
	FText GetCurrentTitleText() const;

	/** Get the title text */
	FText GetTitleText(ENovaMainMenuType MenuType) const;

	/** Get the title color */
	FSlateColor GetCurrentTitleColor() const;

	/** Get the key binding for the previous tab */
	FKey GetPreviousTabKey() const;

	/** Get the key binding for the next tab */
	FKey GetNextTabKey() const;


	/*----------------------------------------------------
		Action callbacks
	----------------------------------------------------*/

protected:

	/** Quit the game */
	void OnClose();


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Settings
	float                                         TooltipDelay;
	float                                         TooltipFadeDuration;
	
	// Tooltip state
	FString                                       DesiredTooltipContent;
	FString                                       CurrentTooltipContent;
	SWidget*                                      CurrentTooltipWidget;
	float                                         CurrentTooltipDelay;
	float                                         CurrentTooltipTime;

	// General state
	bool                                          WasOnMainMenu;

	// Widgets
	TSharedPtr<class SNovaTabView>                TabView;
	TSharedPtr<class SNovaMainMenuHome>           HomeMenu;
	TSharedPtr<class SNovaMainMenuGame>           GameMenu;
	TSharedPtr<class SNovaMainMenuFlight>         FlightMenu;
	TSharedPtr<class SNovaMainMenuAssembly>       AssemblyMenu;
	TSharedPtr<class SNovaMainMenuSettings>       SettingsMenu;


};
