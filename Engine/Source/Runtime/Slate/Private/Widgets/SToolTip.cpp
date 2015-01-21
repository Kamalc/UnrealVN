// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"


static TAutoConsoleVariable<float> StaticToolTipWrapWidth(
	TEXT( "Slate.ToolTipWrapWidth" ),
	1000.0f,
	TEXT( "Width of Slate tool-tips before we wrap the tool-tip text" ) );


float SToolTip::GetToolTipWrapWidth()
{
	return StaticToolTipWrapWidth.GetValueOnGameThread();
}


void SToolTip::Construct( const FArguments& InArgs )
{
	TextContent = InArgs._Text;
	bIsInteractive = InArgs._IsInteractive;
	Font = InArgs._Font;
	ColorAndOpacity = InArgs._ColorAndOpacity;
	TextMargin = InArgs._TextMargin;
	BorderImage = InArgs._BorderImage;
	
	SetContentWidget(InArgs._Content.Widget);
}


void SToolTip::SetContentWidget(const TSharedRef<SWidget>& InContentWidget)
{
	if (InContentWidget != SNullWidget::NullWidget)
	{
		// Widget content argument takes precedence over the text content.
		WidgetContent = InContentWidget;
	}

	TSharedPtr< SWidget > PinnedWidgetContent = WidgetContent.Pin();
	if( PinnedWidgetContent.IsValid() )
	{
		// Tool-tip with entirely custom content.  We'll create a border with some padding (as customized by the user), then
		// embed their custom widget right inside the border.  This tool-tip currently has a different styling than tool-tips
		// that only contain text.
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(BorderImage)
			.Padding(TextMargin)
			[
				PinnedWidgetContent.ToSharedRef()
			]
		];

	}
	else
	{
		// Text-only tool-tip.  This tool-tip currently has a different styling than tool-tips with custom content.  We always want basic
		// text tool-tips to look consistent.
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage( FCoreStyle::Get().GetBrush("ToolTip.BrightBackground") )
			.Padding(FMargin(11.0f))
			[
				SNew(STextBlock)
				.Text(TextContent)
				.Font(FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont"))
				.ColorAndOpacity( FLinearColor::Black )
				.WrapTextAt_Static(&SToolTip::GetToolTipWrapWidth)
			]
		];
	}
}


bool SToolTip::IsEmpty() const
{
	return !WidgetContent.IsValid() && TextContent.Get().IsEmpty();
}


bool SToolTip::IsInteractive() const
{
	return bIsInteractive.Get();
}
