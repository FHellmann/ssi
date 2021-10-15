// Main.cpp
// author: Tobias Baur <tobias.baur@informatik.uni-augsburg.de>
// created: 2017/07/13
// Copyright (C) 2017 University of Augsburg, Tobias Baur
//
// *************************************************************************************************
//
// This file is part of Smart Sensor Integration (SSI) developed at the 
// Lab for Multimedia Concepts and Applications of the University of Augsburg
//
// This library is free software; you can redistribute itand/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or any laterversion.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FORA PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along withthis library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//*************************************************************************************************

#include "ssi.h"
#include "ssibodyfeatures.h"
#include "microsoftkinect2\include\ssimicrosoftkinect2.h"
#include "skeleton\include\ssiskeleton.h"
//#include "xsens\include\ssixsens.h"
//#include "myo\include\ssimyo.h"
#include "signal\include\ssisignal.h"
using namespace ssi;

ssi_char_t sstring[SSI_MAX_CHAR];

void ex_kinect();
void ex_xsens();
void ex_myo();
void test_2D_bodyproperties();

int main() {

#ifdef USE_SSI_LEAK_DETECTOR
	{
#endif

		ssi_print("%s\n\nbuild version: %s\n\n", SSI_COPYRIGHT, SSI_VERSION);

		Factory::RegisterDLL("frame");
		Factory::RegisterDLL("graphic");
		Factory::RegisterDLL("event");
		Factory::RegisterDLL("ioput");
		Factory::RegisterDLL("signal");
		Factory::RegisterDLL("skeleton");
		Factory::RegisterDLL("camera");

		Factory::RegisterDLL("bodyfeatures");

	
		test_2D_bodyproperties();

		ssi_print("\n\n\tpress a key to quit\n");
		getchar();

		Factory::Clear();

#ifdef USE_SSI_LEAK_DETECTOR
	}
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

void test_2D_bodyproperties() {
	ITheFramework* frame = Factory::GetFramework();
	ITheEventBoard* board = Factory::GetEventBoard();

	Decorator* decorator = ssi_create(Decorator, 0, true);
	decorator->getOptions()->icon = true;
	decorator->getOptions()->minmax = true;
	frame->AddDecorator(decorator);

	FileReader* fr = ssi_create(FileReader, 0, true);
	fr->getOptions()->setPath("V:\\nova\\data\\aria-noxi\\001_2016-03-17_Paris\\novice.skel_ssi.stream");


	ITransformable* conv_p = frame->AddProvider(fr, SSI_FILEREADER_PROVIDER_NAME);
	frame->AddSensor(fr);

	BodyProperties2D* bpr = ssi_create(BodyProperties2D, 0, true);
	ITransformable* bpr_t = frame->AddTransformer(conv_p, bpr, "1");
	board->RegisterSender(*bpr);

	SignalPainter* plot_bpr = ssi_create_id(SignalPainter, 0, "plot");
	plot_bpr->getOptions()->setTitle("Body Properties");
	plot_bpr->getOptions()->size = 10;
	plot_bpr->getOptions()->type = PaintSignalType::SIGNAL;
	frame->AddConsumer(bpr_t, plot_bpr, "1");

	SkeletonPainter* skelpaint = ssi_create(SkeletonPainter, 0, true);
	ITransformable* skelpaint_t = frame->AddTransformer(conv_p, skelpaint, "1");

	decorator->add("console", 0, 0, 400, 1000);
	decorator->add("plot*", 400, 0, 800, 1000);

	frame->Start();
	board->Start();

	getchar();

	frame->Stop();
	board->Stop();

	frame->Clear();
	board->Clear();
}

void ex_kinect() {

	//Factory::RegisterDLL("microsoftkinect2.dll");

	ITheFramework *frame = Factory::GetFramework();
	ITheEventBoard *board = Factory::GetEventBoard();

	Decorator *decorator = ssi_create(Decorator, 0, true);
	decorator->getOptions()->icon = true;
	decorator->getOptions()->minmax = true;
	frame->AddDecorator(decorator);
		
	//MicrosoftKinect2 *kinect = ssi_create(MicrosoftKinect2, 0, true);
	//kinect->getOptions()->trackNearestPersons = 1;
	//kinect->getOptions()->showBodyTracking = true;
	//kinect->getOptions()->showFaceTracking = true;
	//
	//ITransformable *rgb_p = frame->AddProvider(kinect, SSI_MICROSOFTKINECT2_RGBIMAGE_PROVIDER_NAME, 0, "2.0s");
	//ITransformable *skeleton_p = frame->AddProvider(kinect, SSI_MICROSOFTKINECT2_SKELETON_PROVIDER_NAME);
	//ITransformable *face3d_p = frame->AddProvider(kinect, SSI_MICROSOFTKINECT2_FACEPOINT3D_PROVIDER_NAME);
	//ITransformable *head_p = frame->AddProvider(kinect, SSI_MICROSOFTKINECT2_HEADPOSE_PROVIDER_NAME);
	//frame->AddSensor(kinect);
	//

	//ITransformable **tf = new ITransformable*[2];
	//tf[0] = head_p;
	//tf[1] = face3d_p;
	//SkeletonConverter* conv = ssi_create(SkeletonConverter, 0, true);
	//conv->getOptions()->n_skeletons = 1;
	//ITransformable *conv_p = frame->AddTransformer(skeleton_p, 2, tf, conv, "1");



	FileReader *fr = ssi_create(FileReader, 0, true);
	fr->getOptions()->setPath("V:\\nova\\data\\aria-noxi\\001_2016-03-17_Paris\\novice.skel_ssi.stream");


	ITransformable *conv_p = frame->AddProvider(fr, SSI_FILEREADER_PROVIDER_NAME);
	frame->AddSensor(fr);







	/*VideoPainter *vplot = ssi_create_id(VideoPainter, 0, "plot");
	vplot->getOptions()->setTitle("skeleton");
	vplot->getOptions()->flip = false;
	frame->AddConsumer(rgb_p, vplot, "1");*/

	
	//
	BodyProperties *bpr = ssi_create(BodyProperties, 0, true);
	ITransformable *bpr_t = frame->AddTransformer(conv_p, bpr, "1");
	board->RegisterSender(*bpr);

	SignalPainter *plot_bpr = ssi_create_id(SignalPainter, 0, "plot");
	plot_bpr->getOptions()->setTitle("Body Properties");
	plot_bpr->getOptions()->size = 10;
	plot_bpr->getOptions()->type = PaintSignalType::SIGNAL;
	frame->AddConsumer(bpr_t, plot_bpr, "1");

	TriggerEventSender *arms_crossed_trigger = ssi_create(TriggerEventSender, 0, true);
	arms_crossed_trigger->getOptions()->triggerType = TriggerEventSender::TRIGGER::GREATER;
	arms_crossed_trigger->getOptions()->thresholdIn = 0.2;
	arms_crossed_trigger->getOptions()->thresholdInEnd = 0.2;
	arms_crossed_trigger->getOptions()->setAddress("PRESENT@armscrossed");
	arms_crossed_trigger->getOptions()->eventType = SSI_ETYPE_MAP;
	frame->AddConsumer(conv_p, arms_crossed_trigger, "1");

	board->RegisterSender(*arms_crossed_trigger);




	//FileWriter *fw = ssi_create(FileWriter, 0, true);
	//fw->getOptions()->setPath("bodyproperties");
	//fw->getOptions()->type = ssi::File::TYPE::ASCII;
 //   frame->AddConsumer(bpr_t, fw, "1.0s");

	//////	
	//EnergyMovement *energy = ssi_create(EnergyMovement, 0, true);
	//ITransformable *energy_t = frame->AddTransformer(conv_p, energy, "1", "14");
	//
	//SignalPainter *plot_energy = ssi_create_id(SignalPainter, 0, "plot");
	//plot_energy->getOptions()->setTitle("Energy Movement");
	//plot_energy->getOptions()->size = 10;
	//plot_energy->getOptions()->type = PaintSignalType::SIGNAL;
	//frame->AddConsumer(energy_t, plot_energy, "1");

	////
	//FluidityMovement *fl = ssi_create(FluidityMovement, 0, true);
	//ITransformable *fl_t = frame->AddTransformer(conv_p, fl, "15", "0.5s");

	//SignalPainter *plot_fluidity = ssi_create_id(SignalPainter, 0, "plot");
	//plot_fluidity->getOptions()->setTitle("Fluidity Movement");
	//plot_fluidity->getOptions()->size = 10;
	//plot_fluidity->getOptions()->type = PaintSignalType::SIGNAL;
	//frame->AddConsumer(fl_t, plot_fluidity, "1");

	////
	//OAMovement *oa = ssi_create(OAMovement, 0, true);
	//ITransformable *oa_t = frame->AddTransformer(conv_p, oa, "1", "5.0s");

	//SignalPainter *plot_oa = ssi_create_id(SignalPainter, 0, "plot");
	//plot_oa->getOptions()->setTitle("OA Movement");
	//plot_oa->getOptions()->size = 10;
	//plot_oa->getOptions()->type = PaintSignalType::SIGNAL;
	//frame->AddConsumer(oa_t, plot_oa, "1");

	////
	//Selector *selector = ssi_create (Selector, "selector", true);
	//ssi_size_t inds[] = {4,8};
	//selector->getOptions()->set(2, inds);
	//ITransformable *select_t = frame->AddTransformer(energy_t,selector,"1");

	//Butfilt *kinect_select_butfilt = ssi_create (Butfilt, "kinect_feat_bufilt", true);
	//ITransformable *kinect_select_butfilt_t = frame->AddTransformer(select_t, kinect_select_butfilt, "1");

	//ThresEventSender *oathres = ssi_create (ThresEventSender, "oathres", true);
	//oathres->getOptions()->setSender("Laughter");
	//oathres->getOptions()->setEvent("ShouldersShake");
	//oathres->getOptions()->hard = true;
	//oathres->getOptions()->mindur = 0.0;
	//oathres->getOptions()->maxdur = 3.0;
	//oathres->getOptions()->hangin = 0;
	//oathres->getOptions()->hangout = 30;
	//oathres->getOptions()->thres = 0.03;
	//oathres->getOptions()->eager = false;
	//oathres->getOptions()->skip = false;
	//frame->AddConsumer(kinect_select_butfilt_t, oathres, "10");
	//board->RegisterSender(*oathres); 

	//Butfilt *kinect_feat_butfilt = ssi_create(Butfilt, "kinect_feat_bufilt", true);
	//ITransformable *kinect_feat_butfilt_t = frame->AddTransformer(energy_t, kinect_feat_butfilt, "1");

	//Openness *se = ssi_create(Openness, 0, true);
	//ITransformable *se_t = frame->AddTransformer(conv_p, se, "1");

	//SignalPainter *se_plot = ssi_create_id(SignalPainter, 0, "plot");
	//se_plot->getOptions()->size = 10;
	//se_plot->getOptions()->type = PaintSignalType::SIGNAL;
	//frame->AddConsumer(se_t, se_plot, "1");

	SkeletonPainter *skelpaint = ssi_create(SkeletonPainter, 0, true);
	ITransformable *skelpaint_t = frame->AddTransformer(conv_p, skelpaint, "1");

	EventMonitor *monitor = ssi_create_id(EventMonitor, 0, "monitor");
	board->RegisterListener(*monitor, "@armscrossed");

	decorator->add("console", 0, 0, 400, 1000);
	decorator->add("plot*", 400, 0, 800, 1000);
	decorator->add("monitor*", 1200, 0, 700, 1000);

	frame->Start();
	board->Start();

	getchar();

	frame->Stop();
	board->Stop();

	frame->Clear();
	board->Clear();

}

