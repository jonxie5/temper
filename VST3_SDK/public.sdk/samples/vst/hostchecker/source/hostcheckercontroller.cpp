//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/hostchecker/source/hostchecker.cpp
// Created by  : Steinberg, 04/2012
// Description :
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2015, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "hostcheckercontroller.h"
#include "eventlogdatabrowsersource.h"

#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "public.sdk/source/vst/vstcomponentbase.h"
#include "public.sdk/source/vst/vstrepresentation.h"

#include "logevents.h"
#include "hostcheckerprocessor.h"

using namespace VSTGUI;

namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
FUID HostCheckerController::cid (0x35AC5652, 0xC7D24CB1, 0xB1427D38, 0xEB690DAF);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::initialize (FUnknown* context)
{
	tresult result = EditControllerEx1::initialize (context);
	if (result == kResultOk)
	{
		// create a unit Latency parameter
		UnitInfo unitInfo;
		unitInfo.id = 1234;
		unitInfo.parentUnitId = kRootUnitId;	// attached to the root unit
		Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name)).assign (USTRING ("Setup"));
		unitInfo.programListId = kNoProgramListId;

		Unit* unit = new Unit (unitInfo);
		addUnit (unit);
		
		parameters.addParameter (STR16 ("Param1"), STR16 (""), 1, 0, ParameterInfo::kCanAutomate, kParam1Tag);
		parameters.addParameter (STR16 ("Param2"), STR16 (""), 0, 0, 0, kParam2Tag);
		parameters.addParameter (STR16 ("Latency"), STR16 (""), 0, 0, 0, kLatencyTag, 1234);

		parameters.addParameter (STR16 ("Bypass"), STR16 (""), 1, 0,
								 ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kBypassTag);

		mDataBrowser = 0;
		mDataSource = 0;

		if (!mDataSource)
			mDataSource = new EventLogDataBrowserSource (this);

		if (!mDataBrowser)
			mDataBrowser =
				new CDataBrowser (CRect (0, 0, 100, 100), mDataSource,
								  CDataBrowser::kDrawRowLines | CDataBrowser::kDrawColumnLines |
									  CDataBrowser::kDrawHeader | CDataBrowser::kVerticalScrollbar);
	}
	return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::terminate ()
{
	tresult result = EditControllerEx1::terminate ();
	if (result == kResultOk)
	{
		mDataSource = 0;
		mDataBrowser = 0;
	}
	return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setComponentState (IBStream* state)
{
	float saved = 0.f;
	if (state->read (&saved, sizeof (float)) != kResultOk)
	{
		return kResultFalse;
	}

#if BYTEORDER == kBigEndian
	SWAP_32 (toSave)
#endif
	if (saved != 12345.67f)
	{
		ASSERT (false)
	}

	uint32 latency;
	if (state->read (&latency, sizeof (uint32)) == kResultOk)
	{
#if BYTEORDER == kBigEndian
		SWAP_32 (latency)
#endif
	}

	uint32 bypass;
	if (state->read (&bypass, sizeof (uint32)) == kResultOk)
	{
#if BYTEORDER == kBigEndian
		SWAP_32 (bypass)
#endif
		setParamNormalized (kBypassTag, bypass > 0 ? 1 : 0);
	}

	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getUnitByBus (MediaType type, BusDirection dir,
														int32 busIndex, int32 channel,
														UnitID& unitId /*out*/)
{
	if (type == kEvent && dir == kInput)
	{
		if (busIndex == 0 && channel == 0)
		{
			unitId = kRootUnitId;
			return kResultTrue;
		}
	}
	addFeatureLog (kLogIdGetUnitByBusSupported);
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setComponentHandler (IComponentHandler* handler)
{
	tresult res = EditControllerEx1::setComponentHandler (handler);
	if (componentHandler2)
	{
		addFeatureLog (kLogIdIComponentHandler2Supported);

		if (componentHandler2->requestOpenEditor () == kResultTrue)
			addFeatureLog (kLogIdIComponentHandler2RequestOpenEditorSupported);
	}

	FUnknownPtr<IComponentHandler3> handler3 (handler);
	if (handler3)
		addFeatureLog (kLogIdIComponentHandler3Supported);

	return res;
}

//-----------------------------------------------------------------------------
int32 PLUGIN_API HostCheckerController::getUnitCount ()
{
	addFeatureLog (kLogIdUnitSupported);
	return EditControllerEx1::getUnitCount ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setParamNormalized (ParamID tag, ParamValue value)
{
	if (tag == kLatencyTag && mLatencyInEdit)
	{
		mWantedLatency = value;
		return kResultTrue;
	}
	return EditControllerEx1::setParamNormalized (tag, value);
}

//------------------------------------------------------------------------
tresult HostCheckerController::beginEdit (ParamID tag)
{
	if (tag == kLatencyTag)
		mLatencyInEdit = true;

	return EditControllerEx1::beginEdit (tag);
}

//-----------------------------------------------------------------------------
tresult HostCheckerController::endEdit (ParamID tag)
{
	if (tag == kLatencyTag && mLatencyInEdit)
	{
		EditControllerEx1::setParamNormalized (tag, mWantedLatency); 
		mLatencyInEdit = false;
	}
	return EditControllerEx1::endEdit (tag);
}

//-----------------------------------------------------------------------------
IPlugView* PLUGIN_API HostCheckerController::createView (FIDString name)
{
	if (ConstString (name) == ViewType::kEditor)
	{
		if (componentHandler2)
		{
			if (componentHandler2->setDirty (true) == kResultTrue)
				addFeatureLog (kLogIdIComponentHandler2SetDirtySupported);
		}

		return new VST3Editor (this, "HostCheckerEditor", "hostchecker.uidesc");
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
CView* HostCheckerController::createCustomView (UTF8StringPtr name, const UIAttributes& attributes,
												const IUIDescription* description, VST3Editor* editor)
{
	if (ConstString (name) == "HostCheckerDataBrowser")
	{
		if (mDataBrowser)
			mDataBrowser->remember ();
		return mDataBrowser;
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::connect (IConnectionPoint* other)
{
	Steinberg::tresult tResult = ComponentBase::connect (other);
	if (peerConnection)
	{
		for (Steinberg::int32 paramIdx = 0; paramIdx < getParameterCount (); ++paramIdx)
		{
			Steinberg::Vst::ParameterInfo paramInfo = {0};
			if (getParameterInfo (paramIdx, paramInfo) == Steinberg::kResultOk)
			{
				IPtr<IMessage> newMsg = owned (allocateMessage ());
				if (newMsg)
				{
					newMsg->setMessageID ("Parameter");
					Steinberg::Vst::IAttributeList* attr = newMsg->getAttributes ();
					if (attr)
					{
						attr->setInt ("ID", paramInfo.id);
					}

					sendMessage (newMsg);
				}
			}
		}

		FUnknownPtr<IAudioProcessor> proc (other);
		if (proc)
		{
			IPtr<IMessage> newMsg = owned (allocateMessage ());
			if (newMsg)
			{
				newMsg->setMessageID ("LogEvent");
				Steinberg::Vst::IAttributeList* attr = newMsg->getAttributes ();
				if (attr)
				{
					attr->setInt ("ID", kLogIdProcessorControllerConnection);
					attr->setInt ("Count", 1);
				}

				notify (newMsg);
			}
		}
	}

	return tResult;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::notify (IMessage* message)
{
	if (!message)
		return kInvalidArgument;

	if (FIDStringsEqual (message->getMessageID (), "LogEvent"))
	{
		LogEvent logEvt;
		if (message->getAttributes ()->getInt ("ID", logEvt.id) != kResultOk)
			return kResultFalse;
		if (message->getAttributes ()->getInt ("Count", logEvt.count) != kResultOk)
			return kResultFalse;

		if (mDataSource && mDataBrowser && mDataSource->updateLog (logEvt))
			mDataBrowser->invalidateRow (logEvt.id);
	}

	if (FIDStringsEqual (message->getMessageID (), "Latency"))
	{
		ParamValue value;
		if (message->getAttributes ()->getFloat ("Value", value) == kResultOk)
		{
			componentHandler->restartComponent (kLatencyChanged);
		}
	}

	return ComponentBase::notify (message);
}

//-----------------------------------------------------------------------------
void HostCheckerController::addFeatureLog (int32 iD)
{
	LogEvent logEvt;
	logEvt.id = iD;
	logEvt.count = 1;

	if (mDataSource && mDataBrowser && mDataSource->updateLog (logEvt))
		mDataBrowser->invalidateRow (logEvt.id);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setKnobMode (KnobMode mode)
{
	addFeatureLog (kLogIdIEditController2Supported);
	return EditControllerEx1::setKnobMode (mode);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::openHelp (TBool onlyCheck)
{
	addFeatureLog (kLogIdIEditController2Supported);
	return EditControllerEx1::openHelp (onlyCheck);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::openAboutBox (TBool onlyCheck)
{
	addFeatureLog (kLogIdIEditController2Supported);
	return EditControllerEx1::openAboutBox (onlyCheck);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::setChannelContextInfos (IAttributeList* list)
{
	if (!list)
		return kResultFalse;

	// optional we can ask for the Channel Name Length
	int64 length;
	if (list->getInt (ChannelContext::kChannelNameLengthKey, length) == kResultTrue)
	{
	}

	// get the Channel Name where we, as Plug-in, are instantiated
	String128 name;
	if (list->getString (ChannelContext::kChannelNameKey, name, sizeof (name)) == kResultTrue)
	{
	}

	// get the Channel UID
	if (list->getString (ChannelContext::kChannelUIDKey, name, sizeof (name)) == kResultTrue)
	{
	}

	// get Channel Index
	int64 index;
	if (list->getInt (ChannelContext::kChannelIndexKey, index) == kResultTrue)
	{
	}

	// get the Channel Color
	int64 color;
	if (list->getInt (ChannelContext::kChannelColorKey, color) == kResultTrue)
	{
		//	ColorSpec channelColor = (ColorSpec)color;
	}

	addFeatureLog (kLogIdChannelContextSupported);

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getXmlRepresentationStream (
	RepresentationInfo& info /*in*/, IBStream* stream /*out*/)
{
	addFeatureLog (kLogIdIXmlRepresentationControllerSupported);

	String name (info.name);
	if (name == GENERIC_8_CELLS)
	{
		Vst::XmlRepresentationHelper helper (info, "Steinberg Media Technologies",
											 "VST3 Host Checker",
											 HostCheckerProcessor::cid.toTUID (), stream);

		helper.startPage ("Main Page");
		helper.startEndCellOneLayer (Vst::LayerType::kKnob, 0);
		helper.startEndCellOneLayer (Vst::LayerType::kKnob, 1);
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.endPage ();

		helper.startPage ("Page 2");
		helper.startEndCellOneLayer (Vst::LayerType::kSwitch, 2);
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty cell
		helper.startEndCell (); // empty
		helper.endPage ();

		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getMidiControllerAssignment (
	int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& id /*out*/)
{
	addFeatureLog (kLogIdIMidiMappingSupported);

	if (busIndex != 0)
		return kResultFalse;

	switch (midiControllerNumber)
	{
		case ControllerNumbers::kCtrlPan: id = kParam1Tag; return kResultOk;
		case ControllerNumbers::kCtrlExpression: id = kParam2Tag; return kResultOk;
		case ControllerNumbers::kCtrlEffect1: id = kBypassTag; return kResultOk;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
int32 PLUGIN_API HostCheckerController::getNoteExpressionCount (int32 busIndex, int16 channel)
{
	addFeatureLog (kLogIdINoteExpressionControllerSupported);
	return 0;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getNoteExpressionInfo (
	int32 busIndex, int16 channel, int32 noteExpressionIndex, NoteExpressionTypeInfo& info /*out*/)
{
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getNoteExpressionStringByValue (
	int32 busIndex, int16 channel, NoteExpressionTypeID id,
	NoteExpressionValue valueNormalized /*in*/, String128 string /*out*/)
{
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API HostCheckerController::getNoteExpressionValueByString (
	int32 busIndex, int16 channel, NoteExpressionTypeID id, const TChar* string /*in*/,
	NoteExpressionValue& valueNormalized /*out*/)
{
	return kResultFalse;
}

//-----------------------------------------------------------------------------
}
} // namespaces
