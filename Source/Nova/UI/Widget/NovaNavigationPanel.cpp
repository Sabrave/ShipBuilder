﻿#pragma once

#include "NovaNavigationPanel.h"
#include "NovaMenu.h"
#include "Nova/Nova.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaNavigationPanel::Construct(const FArguments& InArgs)
{
	Menu = InArgs._Menu;

	ChildSlot[InArgs._Content.Widget];
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaNavigationPanel::ZoomIn()
{
	NLOG("SNovaNavigationPanel::ZoomIn");
}

void SNovaNavigationPanel::ZoomOut()
{
	NLOG("SNovaNavigationPanel::ZoomOut");
}

bool SNovaNavigationPanel::Confirm()
{
	return false;
}

bool SNovaNavigationPanel::Cancel()
{
	return false;
}

TSharedPtr<SNovaButton> SNovaNavigationPanel::GetDefaultFocusButton() const
{
	if (DefaultNavigationButton && DefaultNavigationButton->IsButtonEnabled())
	{
		return DefaultNavigationButton;
	}
	else
	{
		for (TSharedPtr<SNovaButton> Button : NavigationButtons)
		{
			if (Button->IsButtonEnabled())
			{
				return Button;
			}
		}
	}

	return nullptr;
}

TArray<TSharedPtr<SNovaButton>>& SNovaNavigationPanel::GetNavigationButtons()
{
	return NavigationButtons;
}

void SNovaNavigationPanel::ResetNavigation()
{
	TSharedPtr<SNovaButton> FocusButton = GetDefaultFocusButton();
	if (Menu && FocusButton && FocusButton->SupportsKeyboardFocus())
	{
		NLOG("SNovaNavigationPanel::ResetNavigation : reset to '%s'", *FocusButton->ToString());
		Menu->SetFocusedButton(FocusButton, true);
	}
}

bool SNovaNavigationPanel::IsFocusedButtonInsideWidget(TSharedPtr<SWidget> Widget) const
{
	return Menu ? IsButtonInsideWidget(Menu->GetFocusedButton(), Widget) : false;
}

bool SNovaNavigationPanel::IsButtonInsideWidget(TSharedPtr<SNovaButton> Button, TSharedPtr<SWidget> Widget) const
{
	if (Button.IsValid() && NavigationButtons.Contains(Button))
	{
		const FVector2D& OriginPoint = Button->GetCachedGeometry().GetAbsolutePosition();
		const FVector2D& Size        = Button->GetCachedGeometry().GetAbsoluteSize();
		if (Widget->GetCachedGeometry().GetLayoutBoundingRect().ContainsPoint(OriginPoint) ||
			Widget->GetCachedGeometry().GetLayoutBoundingRect().ContainsPoint(OriginPoint + Size))
		{
			return true;
		}
	}

	return false;
}
