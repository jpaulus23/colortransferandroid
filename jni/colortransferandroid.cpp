/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <string.h>
#include <jni.h>

#include <CL/opencl.h>
#include "CLManager.hpp"

#include <android/log.h>

#include "FreeImage/FreeImage.h"

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,"colortransferandroid",__VA_ARGS__)

const char* clProgram =
		"__kernel void rgbToLAB(__read_only image2d_t srcImg, "
	    "               __write_only image2d_t dstImg, "
		"			   sampler_t sampler) { "
		""
		"int2 coords = (int2) (get_global_id(0),get_global_id(1));"
		""
		"/*working: RGB -> LogLMS*/"
		""
		""
		"float4 buf = read_imagef(srcImg,sampler,coords);"
		"float4 buf2 = (float4)(0.0,0.0,0.0,0.0);"
		""
	"float matRGBToLMS[9] = {0.3811, 0.5783, 0.0402,"
	"					    0.1967, 0.7244, 0.0782,"
	"					    0.0241, 0.1228, 0.8444};"
	""
	"buf2.x = matRGBToLMS[0]*buf.x + matRGBToLMS[1]*buf.y + matRGBToLMS[2]*buf.z;"
	"buf2.y = matRGBToLMS[3]*buf.x + matRGBToLMS[4]*buf.y + matRGBToLMS[5]*buf.z;"
	"buf2.z = matRGBToLMS[6]*buf.x + matRGBToLMS[7]*buf.y + matRGBToLMS[8]*buf.z;"
	"buf2.w = 1.0;"
	""
	" /* log LMS color space */ "
	"buf = log10(buf2);"
	""
	" /* do some simple clamping */ "
	"if(buf.x < -12)"
	"	buf.x = -12;"
	"if(buf.y < -12)"
	"	buf.y = -12;"
	"if(buf.z < -12)"
	"	buf.z = -12;"
	""
	"buf.w=1.0;"
	""
	"float matLogLMSToLAB[9] = {0.577350, 0.577350, 0.577350,"
	"						   0.408248, 0.408248, -0.816496,"
	"						   0.707106, -0.707106, 0};"
	""
	"buf2.x = matLogLMSToLAB[0]*buf.x + matLogLMSToLAB[1]*buf.y + matLogLMSToLAB[2]*buf.z;"
	"buf2.y = matLogLMSToLAB[3]*buf.x + matLogLMSToLAB[4]*buf.y + matLogLMSToLAB[5]*buf.z;"
	"buf2.z = matLogLMSToLAB[6]*buf.x + matLogLMSToLAB[7]*buf.y + matLogLMSToLAB[8]*buf.z;"
	""
	"buf2.w = 1.0;"
	""
	"write_imagef(dstImg,coords,buf2);"
	"}"
""
"__kernel void colorTransfer(__read_only image2d_t srcImg,"
"	                        __write_only image2d_t dstImg,"
"							sampler_t sampler,"
"							constant float* data)"
"{"
""
" /* (l*) = l- meanL; */ "
""
"	/*...same for A,B, etc*/"
""
"	/* l' = (stddevTargetL) */ "
"	/*      (-------------) * (l*) */"
"	/*      (stddevSourceL) */ "
""
"	/* ...same for A,B,etc */ "
""
"	/* note: format of data[] = {stddevSourceL,stddevSourceA,stddevSourceB,stddevTargetL,stddevTargetA,stddevTargetB,sourceMeanL,sourceMeanA,sourceMeanB,targetMeanL,targetMeanA,targetMeanB}; */ "
""
	"float stddevSourceL = data[0];"
	"float stddevSourceA = data[1];"
	"float stddevSourceB = data[2];"
""
	"float stddevTargetL = data[3];"
	"float stddevTargetA = data[4];"
	"float stddevTargetB = data[5];"
""
	"float sourceMeanL = data[6];"
	"float sourceMeanA = data[7];"
	"float sourceMeanB = data[8];"
""
	"float targetMeanL = data[9];"
	"float targetMeanA = data[10];"
	"float targetMeanB = data[11];"
""
	"int2 coords = (int2) (get_global_id(0),get_global_id(1));"

	"float4 buf = read_imagef(srcImg,sampler,coords); /* buf is in LAB color space */ "
	"float4 buf2 = (float4) (0.0,0.0,0.0,0.0);"

	" /* do the color transfer in LAB color space */ "
	"buf2.x = (stddevTargetL/stddevSourceL) * (buf.x - sourceMeanL); /* L */ "
	"buf2.y = (stddevTargetA/stddevSourceA) * (buf.y - sourceMeanA); /* A */ "
	"buf2.z = (stddevTargetB/stddevSourceB) * (buf.z - sourceMeanB); /* B */ "
""
	"buf2.x += targetMeanL;"
	"buf2.y += targetMeanA;"
	"buf2.z += targetMeanB;"
""
	"buf2.w = 1.0;"
""
	"float matLABToLogLMS[9] = {0.577350, 0.408248, 0.707106,"
	"						   0.577350, 0.408248, -0.707106,"
	"						   0.577350, -0.816496, 0};"
""
	"buf.x = matLABToLogLMS[0]*buf2.x + matLABToLogLMS[1]*buf2.y + matLABToLogLMS[2]*buf2.z;"
	"buf.y = matLABToLogLMS[3]*buf2.x + matLABToLogLMS[4]*buf2.y + matLABToLogLMS[5]*buf2.z;"
	"buf.z = matLABToLogLMS[6]*buf2.x + matLABToLogLMS[7]*buf2.y + matLABToLogLMS[8]*buf2.z;"
	"buf.w=1;"
""
	" /* logLMS to LMS */ "
	"buf2 = exp10(buf);"
	"buf2.w =1;"
""
	"float matLMSToRGB[9] = {4.4679, -3.5873, 0.1193,"
	"					   -1.2186, 2.3809, -0.1624,"
	"					    0.0497, -0.2439, 1.2045};"
""
	"buf.x = matLMSToRGB[0]*buf2.x + matLMSToRGB[1]*buf2.y + matLMSToRGB[2]*buf2.z;"
	"buf.y = matLMSToRGB[3]*buf2.x + matLMSToRGB[4]*buf2.y + matLMSToRGB[5]*buf2.z;"
	"buf.z = matLMSToRGB[6]*buf2.x + matLMSToRGB[7]*buf2.y + matLMSToRGB[8]*buf2.z;"
	"buf.w = 1;"
""
	"write_imagef(dstImg,coords,buf);"
"}";


#ifdef __cplusplus
extern "C" {
#endif

CLManager clManager;

jobjectArray
Java_com_paulus_colortransferandroid_ColorTransferAndroidActivity_initCL( JNIEnv* env,
                                                  jobject thiz )
{
	clManager.initCL();

	clManager.printDebugInfo();

	unsigned int numStrings = clManager.getDebugStrings()->size();

	 jobjectArray ret;
	 ret= (jobjectArray)env->NewObjectArray(numStrings,
	         env->FindClass("java/lang/String"),
	         env->NewStringUTF(""));

	 for(unsigned int i=0; i<numStrings; i++)
	 {
		 env->SetObjectArrayElement(ret,i,env->NewStringUTF(clManager.getDebugStrings()->at(i).c_str()));
	 }

	 return(ret);
}

jboolean
Java_com_paulus_colortransferandroid_ColorTransferAndroidActivity_buildKernels( JNIEnv* env,
                                                  jobject thiz )
{


	FreeImage_Initialise(); //very important!

	OpenCLContextWrapper* gpuContext=NULL;

	gpuContext = clManager.getCLContext("ARM", "gpu");

	OpenCLCommandQueueWrapper* gpuCommandQueue = NULL;

	cl_int errorCode=-1;

	OpenCLMemoryObjectWrapper* colorTransferDataBuffer;

	if(gpuContext != NULL)
	{
		LOGI("Successfully got Mali GPU opencl context");
		//create program
		OpenCLProgramWrapper* colorTransferProgram = clManager.createCLProgramFromString(gpuContext,gpuContext->childrenDevices[0],clProgram,"colortransfer_program");
		if(!colorTransferProgram)
		{
			LOGI("Error creating program, aborting.");
			return 1;
		}

		//create kernels
		OpenCLKernelWrapper* rgbToLABKernel = clManager.createCLKernel(colorTransferProgram, "rgbToLAB");
		if(!rgbToLABKernel)
		{
			LOGI("Error creating kernel, aborting.");
			return 0;
		}
		OpenCLKernelWrapper* colorTransferKernel = clManager.createCLKernel(colorTransferProgram, "colorTransfer");
		if(!colorTransferKernel)
		{
			LOGI("Error creating kernel, aborting.");
			return 0;
		}

		//samplers
		OpenCLSamplerWrapper* samplerWrapper = clManager.createCLSampler(gpuContext,CL_FALSE,CL_ADDRESS_CLAMP_TO_EDGE,CL_FILTER_NEAREST,"sampler1024clampnearest");

		OpenCLMemoryObjectWrapper* inputImageSource = clManager.loadImage(gpuContext, "/mnt/sdcard/inputsource.png");

		if (!inputImageSource || (inputImageSource->memoryObject == 0))
		{
			LOGI("Error loading: %s","/mnt/sdcard/inputsource.png");
			//Cleanup(context, commandQueue, program, kernel, imageObjects, sampler);
			return 0;
		}

		OpenCLMemoryObjectWrapper* inputImageTarget = clManager.loadImage(gpuContext, "/mnt/sdcard/inputtarget.png");

		if (!inputImageTarget || (inputImageTarget->memoryObject == 0))
		{
			LOGI("Error loading: %s","/mnt/sdcard/inputtarget.png");
			//Cleanup(context, commandQueue, program, kernel, imageObjects, sampler);
			return 0;
		}

		//intermediate buffers needed to store the LAB colorspace image with 32bits per channel
		OpenCLMemoryObjectWrapper* inputImageSource_LAB32 = clManager.createBlankCLImage(gpuContext,CL_MEM_READ_WRITE,CL_RGBA,CL_FLOAT,1024,1024,"inputImageSource_LAB32");
		OpenCLMemoryObjectWrapper* inputImageTarget_LAB32 = clManager.createBlankCLImage(gpuContext,CL_MEM_READ_WRITE,CL_RGBA,CL_FLOAT,1024,1024,"inputImageTarget_LAB32");

		//store the final image in the r32g32b32a32 format (will be downsampled when saving to disk)
		OpenCLMemoryObjectWrapper* outputImage_RGB32 = clManager.createBlankCLImage(gpuContext,CL_MEM_READ_WRITE,CL_RGBA,CL_FLOAT,1024,1024,"outputImage_RGB32");


		//first process the source image
		errorCode  = clSetKernelArg(rgbToLABKernel->kernel,0,sizeof(cl_mem),&inputImageSource->memoryObject);
		errorCode |= clSetKernelArg(rgbToLABKernel->kernel,1,sizeof(cl_mem),&inputImageSource_LAB32->memoryObject);
		errorCode |= clSetKernelArg(rgbToLABKernel->kernel,2,sizeof(cl_sampler),&samplerWrapper->sampler);

		if(errorCode != CL_SUCCESS) {
			LOGI("Error setting kernel arguments, aborting.");
			return 1;
		}

		//grab the command queue so we can deploy the kernel
		gpuCommandQueue = clManager.getCLCommandQueue("Mali-T604");
		if(!gpuCommandQueue) {
			LOGI("Could not get gpuCommandQueue");
			getchar();
			return 1;
		}

		//dimensions of CL workload
		size_t globalWorkSize[2] = {1024,1024}; //(1024x1024)
		size_t localWorkSize[2] = {1,1};

		//rock and roll is here to stay
		CL_ASSERT(clEnqueueNDRangeKernel(gpuCommandQueue->commandQueue,rgbToLABKernel->kernel,2,NULL,globalWorkSize,localWorkSize,0,NULL,NULL));

		//now do the same RGB->LAB conversion for the target image
		errorCode  = clSetKernelArg(rgbToLABKernel->kernel,0,sizeof(cl_mem),&inputImageTarget->memoryObject);
		errorCode |= clSetKernelArg(rgbToLABKernel->kernel,1,sizeof(cl_mem),&inputImageTarget_LAB32->memoryObject);
		errorCode |= clSetKernelArg(rgbToLABKernel->kernel,2,sizeof(cl_sampler),&samplerWrapper->sampler);

		if(errorCode != CL_SUCCESS) {
			LOGI("Error setting kernel arguments, aborting.");
			return 1;
		}

		CL_ASSERT(clEnqueueNDRangeKernel(gpuCommandQueue->commandQueue,rgbToLABKernel->kernel,2,NULL,globalWorkSize,localWorkSize,0,NULL,NULL));

		float* fbufferSource = new float[inputImageSource_LAB32->width * inputImageSource_LAB32->width * 4];
		float* fbufferTarget = new float[inputImageSource_LAB32->width * inputImageSource_LAB32->width * 4];

		size_t origin[3] = {0,0,0};
		size_t region[3] = {inputImageSource_LAB32->width,inputImageSource_LAB32->height,1};

		errorCode = clEnqueueReadImage(gpuCommandQueue->commandQueue,inputImageSource_LAB32->memoryObject,CL_TRUE,origin,region,0,0,fbufferSource,0,NULL,NULL);

		if(errorCode != CL_SUCCESS) {
			LOGI("Error doing clEnqueueReadImage(inputImageSource_LAB32) in main()");
		}

		errorCode = clEnqueueReadImage(gpuCommandQueue->commandQueue,inputImageTarget_LAB32->memoryObject,CL_TRUE,origin,region,0,0,fbufferTarget,0,NULL,NULL);

		if(errorCode != CL_SUCCESS) {
			LOGI("Error doing clEnqueueReadImage(inputImageTarget_LAB32) in main()");
		}

		float sourceMeanL,sourceMeanA,sourceMeanB,targetMeanL,targetMeanA,targetMeanB;
		sourceMeanL=sourceMeanA=sourceMeanB=targetMeanL=targetMeanA=targetMeanB=0.0;

		//compute means
		for(unsigned int i=0; i < (inputImageSource_LAB32->width * inputImageSource_LAB32->height*4); i+=4) {
			sourceMeanL += fbufferSource[i];
			sourceMeanA += fbufferSource[i+1];
			sourceMeanB += fbufferSource[i+2];
		}

		sourceMeanL /= (inputImageSource_LAB32->width * inputImageSource_LAB32->height );
		sourceMeanA /= (inputImageSource_LAB32->width * inputImageSource_LAB32->height );
		sourceMeanB /= (inputImageSource_LAB32->width * inputImageSource_LAB32->height );

		LOGI("sourceMeanL: %f",sourceMeanL);
		LOGI("sourceMeanA: %f",sourceMeanA);
		LOGI("sourceMeanB: %f",sourceMeanB);

		float totalSquaredSourceL=0.0f;
		float totalSquaredSourceA=0.0f;
		float totalSquaredSourceB=0.0f;

		float stddevSourceL=0.0f;
		float stddevSourceA=0.0f;
		float stddevSourceB=0.0f;

		//compute standard deviation
		for(unsigned int i=0; i < (inputImageSource_LAB32->width * inputImageSource_LAB32->height*4); i+=4) {
			//subtract means
			fbufferSource[i] -= sourceMeanL;
			fbufferSource[i+1] -= sourceMeanA;
			fbufferSource[i+2]-= sourceMeanB;

			//compute squares
			fbufferSource[i] *= fbufferSource[i];
			fbufferSource[i+1] *= fbufferSource[i+1];
			fbufferSource[i+2] *= fbufferSource[i+2];

			//add squares
			totalSquaredSourceL += fbufferSource[i];
			totalSquaredSourceA += fbufferSource[i+1];
			totalSquaredSourceB += fbufferSource[i+2];
		}

		stddevSourceL = sqrt(totalSquaredSourceL/((inputImageSource_LAB32->width * inputImageSource_LAB32->height)-1));
		stddevSourceA = sqrt(totalSquaredSourceA/((inputImageSource_LAB32->width * inputImageSource_LAB32->height)-1));
		stddevSourceB = sqrt(totalSquaredSourceB/((inputImageSource_LAB32->width * inputImageSource_LAB32->height)-1));

		LOGI("stddevSourceL: %f " , stddevSourceL );
		LOGI("stddevSourceA: %f" , stddevSourceA );
		LOGI("stddevSourceB: %f" , stddevSourceB );

		//now do the same for the target

		//compute means
		for(unsigned int i=0; i < (inputImageTarget_LAB32->width * inputImageTarget_LAB32->height*4); i+=4) {
			targetMeanL += fbufferTarget[i];
			targetMeanA += fbufferTarget[i+1];
			targetMeanB += fbufferTarget[i+2];
		}

		targetMeanL /= (inputImageTarget_LAB32->width * inputImageTarget_LAB32->height );
		targetMeanA /= (inputImageTarget_LAB32->width * inputImageTarget_LAB32->height );
		targetMeanB /= (inputImageTarget_LAB32->width * inputImageTarget_LAB32->height );

		LOGI("targetMeanL: %f" , targetMeanL );
		LOGI("targetMeanA: %f" , targetMeanA );
		LOGI("targetMeanB: %f" , targetMeanB );

		float totalSquaredTargetL=0.0f;
		float totalSquaredTargetA=0.0f;
		float totalSquaredTargetB=0.0f;

		float stddevTargetL=0.0f;
		float stddevTargetA=0.0f;
		float stddevTargetB=0.0f;

		//compute standard deviation
		for(unsigned int i=0; i < (inputImageTarget_LAB32->width * inputImageTarget_LAB32->height*4); i+=4) {
			//subtract means
			fbufferTarget[i] -= sourceMeanL;
			fbufferTarget[i+1] -= sourceMeanA;
			fbufferTarget[i+2]-= sourceMeanB;

			//compute squares
			fbufferTarget[i] *= fbufferTarget[i];
			fbufferTarget[i+1] *= fbufferTarget[i+1];
			fbufferTarget[i+2] *= fbufferTarget[i+2];

			//add squares
			totalSquaredTargetL += fbufferTarget[i];
			totalSquaredTargetA += fbufferTarget[i+1];
			totalSquaredTargetB += fbufferTarget[i+2];
		}

		stddevTargetL = sqrt(totalSquaredTargetL/((inputImageTarget_LAB32->width * inputImageTarget_LAB32->height)-1));
		stddevTargetA = sqrt(totalSquaredTargetA/((inputImageTarget_LAB32->width * inputImageTarget_LAB32->height)-1));
		stddevTargetB = sqrt(totalSquaredTargetB/((inputImageTarget_LAB32->width * inputImageTarget_LAB32->height)-1));

		LOGI("stddevTargetL: %f", stddevTargetL );
		LOGI("stddevTargetA: %f" , stddevTargetA );
		LOGI("stddevTargetB: %f", stddevTargetB );

		//ok, all required data has been calculated, lets do the color transfer

		float data[] = {stddevSourceL,stddevSourceA,stddevSourceB,stddevTargetL,stddevTargetA,stddevTargetB,sourceMeanL,sourceMeanA,sourceMeanB,targetMeanL,targetMeanA,targetMeanB};

		colorTransferDataBuffer =  clManager.createCLMemoryObject(gpuContext,
												CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
												sizeof(float)*12,
												data,
												"colorTransferDataBuffer");

		errorCode  = clSetKernelArg(colorTransferKernel->kernel,0,sizeof(cl_mem),&inputImageSource_LAB32->memoryObject);
		errorCode |= clSetKernelArg(colorTransferKernel->kernel,1,sizeof(cl_mem),&outputImage_RGB32->memoryObject);
		errorCode |= clSetKernelArg(colorTransferKernel->kernel,2,sizeof(cl_sampler),&samplerWrapper->sampler);
		errorCode |= clSetKernelArg(colorTransferKernel->kernel,3,sizeof(cl_mem) ,&colorTransferDataBuffer->memoryObject);

		if(errorCode != CL_SUCCESS) {
			LOGI("Error setting kernel arguments, aborting.");
			getchar();
			return 1;
		}

		CL_ASSERT(clEnqueueNDRangeKernel(gpuCommandQueue->commandQueue,colorTransferKernel->kernel,2,NULL,globalWorkSize,localWorkSize,0,NULL,NULL));

		//L' =  (stddevTargetL/stddevTargetL) * (L-meanL)

		//clManager.saveImage(inputImageSource_LAB32,"outputSourceLAB.png", gpuCommandQueue);
		//clManager.saveImage(inputImageTarget_LAB32,"outputTargetLAB.png", gpuCommandQueue);
		clManager.saveImage(outputImage_RGB32,"/sdcard/output.png", gpuCommandQueue);

		return 1;
	}

	return 0;
}




#ifdef __cplusplus
}
#endif
