// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FeedbackContext.h"
#include "SNotificationList.h"
#include "INotificationWidget.h"

class SWidgetStack;

/** Feedback context that overrides GWarn for import operations to prevent popup spam */
class SReimportFeedback : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReimportFeedback){}
	SLATE_END_ARGS()

	/** Construct this widget */
	void Construct(const FArguments& InArgs, FText InMainText);

	/** Add a widget to this feedback's widget stack */
	void Add(const TSharedRef<SWidget>& Widget);

	/** Set the main text of this widget */
	void SetMainText(FText InText);
	
	/** Get the main text of this widget */
	FText GetMainText() const;

private:
	/** Cached main text for the notification */
	FText MainText;

	/** The widget stack, displaying contextural information about the current state of the process */
	TSharedPtr<SWidgetStack> WidgetStack;
};

/** Feedback context that overrides GWarn for import operations to prevent popup spam */
class FReimportFeedbackContext : public FFeedbackContext, public INotificationWidget, public TSharedFromThis<FReimportFeedbackContext>
{
public:
	/** Constructor */
	FReimportFeedbackContext();

	/** Initialize this reimport context with the specified widget */
	void Initialize(TSharedRef<SReimportFeedback> Widget);

	/** Destroy this reimport context */
	void Destroy();

	/** Tick this context - potentially opening a new notification if the process has taken a while */
	void Tick();

	TSharedPtr<class SReimportFeedback> GetContent() { return NotificationContent; }

	/** Add a message to the context (and message log) */
	void AddMessage(EMessageSeverity::Type Severity, const FText& Message);

	/** Add a custom widget to the context */
	void AddWidget(const TSharedRef<SWidget>& Widget);

	/** INotificationWidget and FFeedbackContext overrides */
	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) override {}
	virtual void OnSetCompletionState(SNotificationItem::ECompletionState State) override {}
	virtual TSharedRef<SWidget> AsWidget() override { return NotificationContent.ToSharedRef(); };
	virtual void ProgressReported(const float TotalProgressInterp, FText DisplayMessage) override;

private:
	/** Message log for output of errors and log messages */
	FMessageLog MessageLog;

	/** True if we need to notifiy about new messages on the message log */
	bool NotifyMessageLog;

	/** The time when the last change to the cache was reported */
	FTimeLimit ShowNotificationDelay;

	/** The notification that is shown when the context is active */
	TSharedPtr<class SNotificationItem> Notification;

	/** The notification content */
	TSharedPtr<class SReimportFeedback> NotificationContent;
};