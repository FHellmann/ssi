// CMLTrainer.cpp
// author: Johannes Wagner <wagner@hcm-lab.de>
// created: 2016/10/24
// Copyright (C) University of Augsburg, Lab for Human Centered Multimedia
//
// *************************************************************************************************
//
// This file is part of Social Signal Interpretation (SSI) developed at the
// Lab for Human Centered Multimedia of the University of Augsburg
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

#include "CMLTrainer.h"
#include "MongoClient.h"
#include "CMLAnnotation.h"
#include "Annotation.h"
#include "SampleList.h"
#include "ISFlatSample.h"
#include "ISSelectClass.h"
#include "Trainer.h"
#include "ioput/file/FileTools.h"
#include "ffmpeg/include/ssiffmpeg.h"
#include "ioput/include/ssiioput.h"
#include "ssiml.h"
#include "ISTransform.h"
#include "ssi.h"
#include "ssiocv.h"

namespace ssi
{
	ssi_char_t *CMLTrainer::ssi_log_name = "cmltrainer";

	CMLTrainer::CMLTrainer()
		: _ready(false),
		_rootdir(0),
		_scheme(0),
		_stream(0),
		_samples(0),
		_client(0),
		_leftContext(0),
		_rightContext(0),
		_rest_class_id(SSI_SAMPLE_GARBAGE_CLASS_ID)
	{
	}

	CMLTrainer::~CMLTrainer()
	{
		release();
	}

	void printProgress(double percentage)
	{
		#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
		#define PBWIDTH 60

		int val = (int)(percentage * 100);
		int lpad = (int)(percentage * PBWIDTH);
		int rpad = PBWIDTH - lpad;
		printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
		fflush(stdout);
	}


	bool CMLTrainer::init(MongoClient *client,
		const ssi_char_t *rootdir,
		const ssi_char_t *scheme,
		const ssi_char_t *stream,
		ssi_size_t leftContext,
		ssi_size_t rightContext)
	{
		release();

		_client = client;
		_rootdir = ssi_strcpy(rootdir);
		FilePath stream_fp(stream);

		_stream = ssi_strcpy(stream_fp.getPathFull());
			

		_scheme = ssi_strcpy(scheme);
		_leftContext = leftContext;
		_rightContext = rightContext;
		_samples = new SampleList();

		Annotation anno;

		bool externalTraining = false;
		if (ssi_strcmp(stream_fp.getExtension(), SSI_FILE_TYPE_STREAM, false)) {
			if (!CMLAnnotation::SetScheme(&anno, client, _scheme))
			{
				ssi_wrn("scheme not found '%s'", _scheme);
				return false;
			}
		}
		else if (ssi_strcmp(stream_fp.getExtension(), ".mp4", false) || ssi_strcmp(stream_fp.getExtension(), ".avi", false)) //isVideo
		{	
			 //Don't use SSI ordering of class ids
			externalTraining = true;
			if (!CMLAnnotation::SetScheme(&anno, client, _scheme, externalTraining))
			{
				ssi_wrn("scheme not found '%s'", _scheme);
				return false;
			}
		}

		

		if (anno.getScheme()->type != SSI_SCHEME_TYPE::DISCRETE
			&& anno.getScheme()->type != SSI_SCHEME_TYPE::CONTINUOUS)
		{
			ssi_wrn("scheme is not continuous or discrete");
			return false;
		}

		if (anno.getScheme()->type == SSI_SCHEME_TYPE::DISCRETE)
		{
			anno.addClass(SSI_SAMPLE_REST_CLASS_NAME, _rest_class_id, externalTraining);
			for (ssi_size_t i = 0; i < anno.getScheme()->discrete.n; i++)
			{
				_samples->addClassName(anno.getScheme()->discrete.names[i]);
			}
			
		}
		else if (anno.getScheme()->type == SSI_SCHEME_TYPE::CONTINUOUS)
		{
		}
		else
		{
			ssi_wrn("scheme is not supported '%s'", SSI_SCHEME_NAMES[anno.getScheme()->type]);
			return false;
		}

		_ready = true;

		return true;
	}

	bool CMLTrainer::collect(const ssi_char_t *session,
		const ssi_char_t *role,
		const ssi_char_t *annotator,
		bool cooperative,
		ssi_video_params_t &video_params,
		double cmlbegintime = 0
		)
	{
		if (!_ready)
		{
			ssi_wrn("call init() first");
			return false;
		}

		ssi_char_t path[SSI_MAX_CHAR];

		ssi_sprint(path, "%s\\%s\\%s.%s", _rootdir, session, role, _stream);
		if (!ssi_exists(path))
		{
			ssi_wrn("stream not found '%s'", path);
			return false;
		}



		bool externalTraining = false;
		ssi_stream_t stream;
		FilePath stream_fp(path);
		if (ssi_strcmp(stream_fp.getExtension(), SSI_FILE_TYPE_STREAM, false)) {
			if (!FileTools::ReadStreamFile(path, stream))
			{
				ssi_wrn("could not load stream '%s'", path);
				return 0;
			}
			ssi_video_params(video_params, 0, 0, 0, 0, 0);
		}
		else if (ssi_strcmp(stream_fp.getExtension(), ".mp4", false) || ssi_strcmp(stream_fp.getExtension(), ".avi", false)) //isVideo
		{



			try {
				ssi_print("%s\n", path);
				cv::VideoCapture cap(path);
				if (!cap.isOpened())
					ssi_wrn("Can not open Video file (Maybe opencv_ffmpeg310_x64.dll or mp4 codec is missing.)");

				double fps = cap.get(CV_CAP_PROP_FPS);
				double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
				double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
				double num = cap.get(CV_CAP_PROP_FRAME_COUNT);
			    ssi_video_params(video_params, width, height, fps, 8, 3);
				

				cv::Mat frame;
				int size = ssi_video_size(video_params);
				ssi_stream_init(stream, num, 1, size, SSI_IMAGE, fps);
				stream.num = 0;
				stream.tot = 0;
				stream.byte = size;
				ssi_stream_t temp;
				ssi_stream_init(temp, 1, 1, size, SSI_IMAGE, fps, 0);
				externalTraining = true;
				for (int i = 0; i < cap.get(CV_CAP_PROP_FRAME_COUNT); i++)
				{
				
					
					cap >> frame;
					
					temp.byte = size;
					temp.tot = temp.byte * 1;

					printProgress(((float)i / (float)cap.get(CV_CAP_PROP_FRAME_COUNT)));
					
					ssi_byte_t *bytes = new ssi_byte_t[size];
					memcpy(bytes, frame.data, size);
					temp.ptr = bytes;
				    ssi_stream_cat(temp, stream);
					
					delete[] bytes;
					
				}
				ssi_stream_destroy(temp);


				cap.release();

			}
			catch (cv::Exception& e) {
				ssi_print("%s", e.msg);
				return false;
			}
		}

		ssi_time_t frame = 1 / stream.sr;

		Annotation anno;
		if (!CMLAnnotation::Load(&anno, _client, session, role, _scheme, annotator, externalTraining))
		{
			ssi_wrn("annotation not found '%s.%s.%s.%s'", session, role, _scheme, annotator);
			return false;
		}

		if (anno.size() == 0)
		{
			ssi_wrn("annotation is empty '%s.%s.%s.%s'", session, role, _scheme, annotator);
			return false;
		}

		if (anno.getScheme()->type == SSI_SCHEME_TYPE::DISCRETE)
		{
			if (cooperative)
			{
				ssi_time_t last_to = 0;
				if (cmlbegintime > 0) last_to = cmlbegintime;
				else  last_to = (anno.end() - 1)->discrete.to;
				anno.filter(last_to, Annotation::FILTER_PROPERTY::TO, Annotation::FILTER_OPERATOR::LESSER_EQUAL);
			}





			ssi_time_t duration = stream.num / stream.sr - frame;
			anno.addClass(_rest_class_id, SSI_SAMPLE_REST_CLASS_NAME, externalTraining);
			if (!anno.convertToFrames(frame, SSI_SAMPLE_REST_CLASS_NAME, duration))
			{
				return false;
			}

			//const ssi_scheme_t *s = anno.getScheme();
			//for (ssi_size_t i = 0; i < s->discrete.n; i++)
			//{
			//	ssi_print("%s -> %d\n", s->discrete.names[i], s->discrete.ids[i]);
			//}

			if (_leftContext > 0 || _rightContext > 0)
			{
				anno.addOffset(-(_leftContext*frame), _rightContext*frame);
				anno.filter(0, Annotation::FILTER_PROPERTY::FROM, Annotation::FILTER_OPERATOR::GREATER_EQUAL);
				anno.filter(duration, Annotation::FILTER_PROPERTY::TO, Annotation::FILTER_OPERATOR::LESSER);
			}

			if (!anno.extractSamples(stream, _samples, session))
			{
				return false;
			}
		}
		else if (anno.getScheme()->type == SSI_SCHEME_TYPE::CONTINUOUS)
		{
			int cmlbeginframe = 0;
			if (cooperative)
			{
				if (cmlbegintime > 0)
				{
					cmlbeginframe = anno.getScheme()->continuous.sr * cmlbegintime;
				}
			}
			anno.extractSamplesFromContinuousScheme(stream, _samples, _leftContext, _rightContext, session, cmlbeginframe);
		}
		else
		{
			ssi_wrn("scheme is not supported '%s'", SSI_SCHEME_NAMES[anno.getScheme()->type]);
			return false;
		}

		ssi_stream_destroy(stream);

		return true;
	}

	bool CMLTrainer::collect_multi(const ssi_char_t *session,
		const ssi_char_t *role,
		const ssi_char_t *annotator,
		const ssi_char_t *stream_path, 
		const ssi_char_t *root_dir, 
		MongoClient *client)
	{
		if (!_ready)
		{
			ssi_wrn("call init() first");
			return false;
		}

		ssi_char_t path[SSI_MAX_CHAR];

		ssi_sprint(path, "%s\\%s\\%s.%s", root_dir, session, role, stream_path);
		if (!ssi_exists(path))
		{
			ssi_wrn("stream not found '%s'", path);
			return false;
		}

		ssi_stream_t stream;
		if (!FileTools::ReadStreamFile(path, stream))
		{
			ssi_wrn("could not load stream '%s'", path);
			return false;
		}
		ssi_time_t frame = 1 / stream.sr;

		Annotation anno;
		if (!CMLAnnotation::Load(&anno, client, session, role, _scheme, annotator))
		{
			ssi_wrn("annotation not found '%s.%s.%s.%s'", session, role, _scheme, annotator);
			return false;
		}

		if (anno.size() == 0)
		{
			ssi_wrn("annotation is empty '%s.%s.%s.%s'", session, role, _scheme, annotator);
			return false;
		}

		if (anno.getScheme()->type == SSI_SCHEME_TYPE::DISCRETE)
		{
			ssi_time_t duration = stream.num / stream.sr - frame;
			anno.addClass(_rest_class_id, SSI_SAMPLE_REST_CLASS_NAME);
			if (!anno.convertToFrames(frame, SSI_SAMPLE_REST_CLASS_NAME, duration))
			{
				return false;
			}

			if (_leftContext > 0 || _rightContext > 0)
			{
				anno.addOffset(-(_leftContext*frame), _rightContext*frame);
				anno.filter(0, Annotation::FILTER_PROPERTY::FROM, Annotation::FILTER_OPERATOR::GREATER_EQUAL);
				anno.filter(duration, Annotation::FILTER_PROPERTY::TO, Annotation::FILTER_OPERATOR::LESSER);
			}

			if (!anno.extractSamples(stream, _samples, session))
			{
				return false;
			}
		}
		else if (anno.getScheme()->type == SSI_SCHEME_TYPE::CONTINUOUS)
		{
			anno.extractSamplesFromContinuousScheme(stream, _samples, _leftContext, _rightContext, session);
		}
		else
		{
			ssi_wrn("scheme is not supported '%s'", SSI_SCHEME_NAMES[anno.getScheme()->type]);
			return false;
		}

		ssi_stream_destroy(stream);

		return true;
	}

	bool CMLTrainer::train(Trainer *trainer)
	{
		if (_samples->getSize() == 0)
		{

			//If we have an empty samplelist at this point, we check if the training is done externally in the trainer.
			return trainer->train(*_samples);
		}

		else {

			ssi_size_t rest_index = _samples->addClassName(SSI_SAMPLE_REST_CLASS_NAME);
			if (_samples->getSize(rest_index) == 0)
			{
				ISSelectClass select(_samples);
				select.setSelectionInverse(rest_index);
				ISFlatSample flat(&select);
				return trainer->train(flat);
			}
			else
			{
				ISFlatSample flat(_samples);
				return trainer->train(flat);
			}
		}
	}


	

	bool CMLTrainer::eval(Trainer *trainer, const ssi_char_t *evalpath, bool crossval, ssi_video_params_t video_params)
	{
		if (!trainer->isTrained())
		{
			ssi_wrn("train trainer first");
			return false;
		}

		if (_samples->getSize() == 0)
		{
			ssi_wrn("collect samples first");
			return false;
		}

		FILE *fp = ssi_fopen(evalpath, "w");
		if (!fp)
		{
			ssi_wrn("could not open file '%s'", evalpath);
			return false;
		}

		ssi_size_t rest_index = _samples->addClassName(SSI_SAMPLE_REST_CLASS_NAME);

	
		if (video_params.numOfChannels == 0)
		{

			if (_samples->getSize(rest_index) == 0)
			{
				ISSelectClass select(_samples);
				select.setSelectionInverse(rest_index);
				ISFlatSample flat(&select);

				
				if (crossval)
				{
					trainer->evalLOUO(flat, fp, Evaluation::PRINT::CSV);
				}
				else
				{
					trainer->eval(flat, fp, Evaluation::PRINT::CSV);
				}
			}
			else
			{
				ISFlatSample flat(_samples);
				if (crossval)
				{
					trainer->evalLOUO(flat, fp, Evaluation::PRINT::CSV);
				}
				else
				{
					trainer->eval(flat, fp, Evaluation::PRINT::CSV);
				}
			}
		}

		else
		{
			try {
			
				ISFlatSample flat(_samples);
				trainer->eval(flat, video_params, fp, Evaluation::PRINT::CSV);
				
			}
			catch (cv::Exception& e) {
				ssi_print("%s", e.msg);
				return false;
			}


		}//isVideo


		return true;
	}



	Annotation *CMLTrainer::forward(Trainer *trainer,
		const ssi_char_t *session,
		const ssi_char_t *role,
		const ssi_char_t *annotator,
		bool cooperative,
		double cmlbegintime)
	{

		bool externalTraining = false;
		if (!_ready)
		{
			ssi_wrn("call init() first");
			return 0;
		}

		ssi_char_t path[SSI_MAX_CHAR];

		ssi_sprint(path, "%s\\%s\\%s.%s", _rootdir, session, role, _stream);
		if (!ssi_exists(path))
		{
			ssi_wrn("stream not found '%s'", path);
			return 0;
		}

		ssi_stream_t stream;
		FilePath stream_fp(path);
		if (ssi_strcmp(stream_fp.getExtension(), SSI_FILE_TYPE_STREAM, false)) {
			if (!FileTools::ReadStreamFile(path, stream))
			{
				ssi_wrn("could not load stream '%s'", path);
				return 0;
			}
		
			}
		else if (ssi_strcmp(stream_fp.getExtension(), ".mp4", false) || ssi_strcmp(stream_fp.getExtension(), ".avi", false)) //isVideo
		{

	
		
		try {
			ssi_print("%s\n", path);
			cv::VideoCapture cap(path);
			if (!cap.isOpened())  
				ssi_wrn("Can not open Video file (Maybe opencv_ffmpeg310_x64.dll or mp4 codec is missing.)");

			double fps = cap.get(CV_CAP_PROP_FPS);
			double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
			double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
			double num = cap.get(CV_CAP_PROP_FRAME_COUNT);
			ssi_video_params_t video_format;
			ssi_video_params(video_format, width, height, fps, 8, 3);
			ssi_stream_init(stream, num, 1, ssi_video_size(video_format), SSI_IMAGE, fps);
			stream.num = num;
			stream.type = SSI_IMAGE;
			stream.sr = fps;
			stream.dim = 1;
			stream.byte = ssi_video_size(video_format);
			cap.release();
			externalTraining = true;
		

		}
		catch (cv::Exception& e) {
			ssi_print("%s", e.msg);
			return false;
		}

		}

		ssi_time_t frame = 1 / stream.sr;

		Annotation *anno = new Annotation();
		ssi_time_t last_to_time = 0;
		ssi_size_t last_to_count = 0;

		if (cooperative)
		{
			if (!CMLAnnotation::Load(anno, _client, session, role, _scheme, annotator))
			{
				ssi_wrn("annotation not found '%s.%s.%s.%s'", session, role, _scheme, annotator);
				return 0;
			}

			if (anno->size() == 0)
			{
				ssi_wrn("annotation is empty '%s.%s.%s.%s'", session, role, _scheme, annotator);
				return 0;
			}

			if (anno->getScheme()->type == SSI_SCHEME_TYPE::CONTINUOUS)
			{
				if (cmlbegintime > 0) last_to_count = cmlbegintime * anno->getScheme()->continuous.sr;
				else
				{
					last_to_count = (ssi_size_t)anno->size();
					for (Annotation::reverse_iterator it = anno->rbegin(); it != anno->rend(); it++)
					{
						if (!isnan(it->continuous.score))
						{
							break;
						}
						--last_to_count;
					}
				}
			}
			else
			{
				if (cmlbegintime > 0) last_to_time = cmlbegintime;
				else last_to_time = (anno->end() - 1)->discrete.to;
			}
		}
		else
		{
			if (!CMLAnnotation::SetScheme(anno, _client, _scheme, externalTraining))
			{
				ssi_wrn("scheme not found '%s'", _scheme);
				return 0;
			}
			anno->setMeta("annotator", annotator);
			anno->setMeta("role", role);
		}

		ssi_time_t duration = stream.num / stream.sr - frame;
		

		if (anno->getScheme()->type == SSI_SCHEME_TYPE::DISCRETE)
		{
			if (!anno->convertToFrames(frame, SSI_SAMPLE_REST_CLASS_NAME, duration))
			{
				return 0;
			}
			
			if (_leftContext > 0 || _rightContext > 0)
			{
				anno->addOffset(-(_leftContext*frame), _rightContext*frame);
				anno->filter(0, Annotation::FILTER_PROPERTY::FROM, Annotation::FILTER_OPERATOR::GREATER_EQUAL);
				anno->filter(duration, Annotation::FILTER_PROPERTY::TO, Annotation::FILTER_OPERATOR::LESSER);
			}
			

			ssi_size_t n_classes = trainer->getClassSize();
			ssi_int_t *class_map = new ssi_int_t[n_classes];
			for (ssi_size_t i = 0; i < n_classes; i++)
			{
				if (!anno->getClassId(trainer->getClassName(i), class_map[i]))
				{
					ssi_wrn("class not found '%s'", trainer->getClassName(i));
					return 0;
				}
			}
			
			
		
		
			ssi_stream_t chunk = stream;
			ssi_size_t n_bytes = stream.byte * stream.dim;
			ssi_size_t index;
			ssi_real_t confidence;


			if (ssi_strcmp(stream_fp.getExtension(), SSI_FILE_TYPE_STREAM, false)) {

			for (Annotation::iterator it = anno->begin(); it != anno->end(); it++)
			{
				if (cooperative && !(it->discrete.from > last_to_time))
				{
					continue;
				}

				ssi_size_t from = ssi_cast(ssi_size_t, it->discrete.from * stream.sr + 0.5);
				ssi_size_t to = ssi_cast(ssi_size_t, it->discrete.to * stream.sr + 0.5);

				// check if samples start index is smaller than the stop index and smaller than streams number of samples
				if (!(from <= to && to < stream.num)) {
					ssi_wrn("interval out of range [%lf..%lf]s", it->discrete.from, it->discrete.to);
					continue;
				}

				chunk.ptr = stream.ptr + from * n_bytes;
				chunk.dim = fmax(1, to - from) * stream.dim;
				chunk.num = 1;
				chunk.tot = chunk.num * n_bytes;

				
				trainer->forward(chunk, index, confidence);
				it->discrete.id = class_map[index];
				it->confidence = confidence;

			}



			}

			else if (ssi_strcmp(stream_fp.getExtension(), ".mp4", false) || ssi_strcmp(stream_fp.getExtension(), ".avi", false))
			{

				Annotation::iterator it = anno->begin();
				
			

				try {
					ssi_print("%s\n", path);
					cv::VideoCapture cap(path);
					if (!cap.isOpened())
						ssi_wrn("Can not open Video file");

					double fps = cap.get(CV_CAP_PROP_FPS);
					double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
					double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
					double num = cap.get(CV_CAP_PROP_FRAME_COUNT);
					ssi_video_params_t video_format;
					ssi_video_params(video_format, width, height, fps, 8, 3);
					

					
					ssi_print("Predicting frames..");
					cv::Mat frame;
					ssi_stream_t chunk;
					ssi_stream_init(chunk, 1, 1, ssi_video_size(video_format), SSI_IMAGE, video_format.framesPerSecond);
					for (int i = 0; i < cap.get(CV_CAP_PROP_FRAME_COUNT); i++)
					{
					
						ssi_size_t from = ssi_cast(ssi_size_t, it->discrete.from * stream.sr + 0.5);
						ssi_size_t to = ssi_cast(ssi_size_t, it->discrete.to * stream.sr + 0.5);

						// check if samples start index is smaller than the stop index and smaller than streams number of samples
						if (!(from <= to && to < stream.num)) {
							ssi_wrn("interval out of range [%lf..%lf]s", it->discrete.from, it->discrete.to);
							continue;
						}

						
						cap >> frame;

						if (cooperative && !(it->discrete.from > last_to_time))
						{
							it++;
							continue;
						}
			
						

						printProgress(((float)i / (float)cap.get(CV_CAP_PROP_FRAME_COUNT)));
						int size = frame.total() * frame.channels();
						ssi_byte_t *bytes = new ssi_byte_t[size];
						memcpy(bytes, frame.data, size);

						chunk.byte = size;

						chunk.ptr = bytes;
						chunk.tot = chunk.num * size;
						trainer->forward(chunk, index, confidence, video_format);
	
						
						it->discrete.id = class_map[index];
						it->confidence = confidence;
						it++;
	    				delete[] bytes;
		
						//ssi_stream_destroy(chunk);
					}
				
					cap.release();


				}
				catch (cv::Exception& e) {
					ssi_print("%s", e.msg);
					return false;
				}

			}
			

			anno->addOffset((frame*_leftContext), -(frame*_rightContext));
			anno->removeClass(SSI_SAMPLE_REST_CLASS_NAME);

			delete[] class_map;
			
		}
		else if (anno->getScheme()->type == SSI_SCHEME_TYPE::CONTINUOUS)
		{

			if (ssi_strcmp(stream_fp.getExtension(), SSI_FILE_TYPE_STREAM, false)) {


				ssi_size_t n_bytes = stream.byte * stream.dim;
				ssi_size_t index;
				ssi_real_t score;

				ssi_size_t from = cooperative ? last_to_count : 0;
				ssi_size_t to = stream.num - _leftContext - _rightContext;

				ssi_stream_t chunk = stream;
				chunk.num = 1;
				chunk.ptr += from * n_bytes;
				chunk.dim = stream.dim + stream.dim * _leftContext + stream.dim * _rightContext;
				chunk.tot = chunk.dim * chunk.byte;

				ssi_size_t old_size = (ssi_size_t)anno->size();
				ssi_real_t min_score = anno->getScheme()->continuous.min;
				ssi_real_t max_score = anno->getScheme()->continuous.max;
				ssi_real_t confidence = 0.0f;
				for (ssi_size_t i = from; i < to; i++)
				{
					trainer->forward(chunk, index, score, confidence);
					if (score < min_score)
					{
						score = min_score;
					}
					else if (score > max_score)
					{
						score = max_score;
					}

					if (from < old_size)
					{
						anno->at(i).continuous.score = score;
						anno->at(i).confidence = confidence;
					}
					else
					{
						anno->add(score, confidence);
					}
					chunk.ptr += n_bytes;
				}
			}

			else if (ssi_strcmp(stream_fp.getExtension(), ".mp4", false) || ssi_strcmp(stream_fp.getExtension(), ".avi", false))
			{


				ssi_size_t n_bytes = stream.byte * stream.dim;
				ssi_size_t index;
				ssi_real_t score;

				ssi_size_t from = cooperative ? last_to_count : 0;
				ssi_size_t to = stream.num - _leftContext - _rightContext;

			
				ssi_size_t old_size = (ssi_size_t)anno->size();
				ssi_real_t min_score = anno->getScheme()->continuous.min;
				ssi_real_t max_score = anno->getScheme()->continuous.max;
				ssi_real_t confidence = 0.0f;





				ssi_print("%s\n", path);
				cv::VideoCapture cap(path);
				if (!cap.isOpened())
					ssi_wrn("Can not open Video file");

				double fps = cap.get(CV_CAP_PROP_FPS);
				double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
				double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
				double num = cap.get(CV_CAP_PROP_FRAME_COUNT);
				ssi_video_params_t video_format;
				ssi_video_params(video_format, width, height, fps, 8, 3);



				ssi_print("Predicting frames..");
				cv::Mat frame;
				ssi_stream_t chunk;
				ssi_stream_init(chunk, 1, 1, ssi_video_size(video_format), SSI_IMAGE, video_format.framesPerSecond);
				for (int i = from ; i < cap.get(CV_CAP_PROP_FRAME_COUNT); i++)
				{

					cap >> frame;
					printProgress(((float)i / (float)cap.get(CV_CAP_PROP_FRAME_COUNT)));
					int size = frame.total() * frame.channels();
					ssi_byte_t *bytes = new ssi_byte_t[size];
					memcpy(bytes, frame.data, size);

					chunk.byte = size;

					chunk.ptr = bytes;
					chunk.tot = chunk.num * size;

					trainer->forward(chunk, index, score, confidence, video_format);
					if (score < min_score)
					{
						score = min_score;
					}
					else if (score > max_score)
					{
						score = max_score;
					}

					if (from < old_size)
					{
						anno->at(i).continuous.score = score;
						anno->at(i).confidence = confidence;
					}
					else
					{
						anno->add(score, confidence);
					}
					chunk.ptr += n_bytes;


					//trainer->forward(chunk, index, confidence, video_format);

					delete[] bytes;

				}

				cap.release();

			}


		}
		else
		{
			ssi_wrn("scheme is not supported '%s'", SSI_SCHEME_NAMES[anno->getScheme()->type]);
			return false;
		}

		

		ssi_stream_destroy(stream);

		return anno;

		
		
	}

	void CMLTrainer::release()
	{
		delete[] _rootdir; _rootdir = 0;
		delete[] _stream; _stream = 0;
		delete _samples; _samples = 0;
		delete[] _scheme; _scheme = 0;
		_rest_class_id = SSI_SAMPLE_GARBAGE_CLASS_ID;
		_client = 0;
		_ready = false;
	}
}