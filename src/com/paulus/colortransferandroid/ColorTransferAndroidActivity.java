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
 */
package com.paulus.colortransferandroid;


import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.app.Activity;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;
import android.hardware.Camera.PreviewCallback;
import android.graphics.ImageFormat;
import android.graphics.YuvImage;


public class ColorTransferAndroidActivity extends Activity
{
	private SurfaceView preview=null;
	private SurfaceHolder previewHolder=null;
	private Camera camera=null;
	private boolean inPreview=false;
	private boolean cameraConfigured=false;

    //native methods
    public native String[] initCL();
    public native boolean buildKernels();
    public native boolean loadSourceImage();
    public native boolean loadTargetImage();
    public native boolean performColorTransfer();
    
    static
    {
    	System.load("/system/vendor/lib/egl/libGLES_mali.so");
        System.loadLibrary("colortransferandroid");
        
    }
    
    @Override	
	public void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);

		setContentView(R.layout.main);

		preview=(SurfaceView)findViewById(R.id.cameraPreview);
		previewHolder=preview.getHolder();
		previewHolder.addCallback(surfaceCallback);
		
		//opencl debugging
		String stringArray[] = initCL();	
		
		ListView listView1 = (ListView) findViewById(R.id.openclListView);
		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                 android.R.layout.simple_list_item_1, stringArray);     
		listView1.setAdapter(adapter);
		
		//OpenCL work
		buildKernels();
		loadSourceImage();
		loadTargetImage();
		performColorTransfer();
	}	

	@Override
	public void onResume()
	{
		super.onResume();

		camera=Camera.open();
		startPreview();
	}

	@Override
	public void onPause()
	{
		if (inPreview)
		{
			camera.stopPreview();
		}

		camera.release();
		camera=null;
		inPreview=false;

		super.onPause();
	}

	private Camera.Size getBestPreviewSize(int width, int height,
										   Camera.Parameters parameters) 
	{
		Camera.Size result=null;

		for (Camera.Size size : parameters.getSupportedPreviewSizes())
		{
			if (size.width<=width && size.height<=height)
			{
				if (result==null) 
				{
					result=size;
				}
				else 
				{
					int resultArea=result.width*result.height;
					int newArea=size.width*size.height;

					if (newArea>resultArea)
					{
						result=size;
					}
				}
			}
		}

		return(result);
	}

	private void initPreview(int width, int height)
	{
		if (camera!=null && previewHolder.getSurface()!=null)
			{
			try
			{
				camera.setPreviewDisplay(previewHolder);
				/*
				camera.setPreviewCallback(new PreviewCallback() 
				{
					public void onPreviewFrame(byte[] data, Camera camera) 
					{
						Camera.Parameters parameters = camera.getParameters();
						int format = parameters.getPreviewFormat();

						//YUV formats require more conversion
						if (format == ImageFormat.NV21)// || format == ImageFormat.YUY2 || format == ImageFormat.NV16)
						{
							int w = parameters.getPreviewSize().width;
							int h = parameters.getPreviewSize().height;
							// Get the YuV image
							YuvImage yuv_image = new YuvImage(data, format, w, h, null);
							// Convert YuV to Jpeg
							//Rect rect = new Rect(0, 0, w, h);
							//ByteArrayOutputStream output_stream = new ByteArrayOutputStream();
							//yuv_image.compressToJpeg(rect, 100, output_stream);
							//byte[] byt=output_stream.toByteArray();
						}
					}
				});
				*/
			}
			catch (Throwable t)
			{
				Log.e("PreviewDemo-surfaceCallback",
					  "Exception in setPreviewDisplay()", t);
				Toast
					.makeText(ColorTransferAndroidActivity.this, t.getMessage(), Toast.LENGTH_LONG)
					.show();
			}

			if (!cameraConfigured)
			{
				Camera.Parameters parameters=camera.getParameters();
				Camera.Size size=getBestPreviewSize(width, height,
													parameters);

				if (size!=null)
				{
					parameters.setPreviewSize(size.width, size.height);
					camera.setParameters(parameters);
					cameraConfigured=true;
				}
			}
		}
	}

	private void startPreview() 
	{
		if (cameraConfigured && camera!=null)
		{
			camera.startPreview();
			inPreview=true;
		}
	}

	SurfaceHolder.Callback surfaceCallback=new SurfaceHolder.Callback()
	{
		public void surfaceCreated(SurfaceHolder holder)
		{
			// no-op -- wait until surfaceChanged()
		}

		public void surfaceChanged(SurfaceHolder holder,
								   int format, int width,
								   int height) 
		{
			initPreview(width, height);
			startPreview();
		}

		public void surfaceDestroyed(SurfaceHolder holder) 
		{
			// no-op
		}
	};
}

/*
 * 
import android.app.Activity;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;
import android.hardware.Camera.PreviewCallback;
import android.graphics.ImageFormat;
import android.graphics.YuvImage;

public class MainActivity extends Activity
{
	private SurfaceView preview=null;
	private SurfaceHolder previewHolder=null;
	private Camera camera=null;
	private boolean inPreview=false;
	private boolean cameraConfigured=false;

	@Override
	public void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);

		setContentView(R.layout.main);

		preview=(SurfaceView)findViewById(R.id.cpPreview);
		previewHolder=preview.getHolder();
		previewHolder.addCallback(surfaceCallback);
		previewHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
	}

	@Override
	public void onResume()
	{
		super.onResume();

		camera=Camera.open();
		startPreview();
	}

	@Override
	public void onPause()
	{
		if (inPreview)
		{
			camera.stopPreview();
		}

		camera.release();
		camera=null;
		inPreview=false;

		super.onPause();
	}

	private Camera.Size getBestPreviewSize(int width, int height,
										   Camera.Parameters parameters) 
	{
		Camera.Size result=null;

		for (Camera.Size size : parameters.getSupportedPreviewSizes())
		{
			if (size.width<=width && size.height<=height)
			{
				if (result==null) 
				{
					result=size;
				}
				else 
				{
					int resultArea=result.width*result.height;
					int newArea=size.width*size.height;

					if (newArea>resultArea)
					{
						result=size;
					}
				}
			}
		}

		return(result);
	}

	private void initPreview(int width, int height)
	{
		if (camera!=null && previewHolder.getSurface()!=null)
			{
			try
			{
				camera.setPreviewDisplay(previewHolder);
				camera.setPreviewCallback(new PreviewCallback() 
				{
					// Called for each frame previewed
					@SuppressWarnings("null")

					public void onPreviewFrame(byte[] data, Camera camera) 
					{
						Camera.Parameters parameters = camera.getParameters();
						int format = parameters.getPreviewFormat();

						//YUV formats require more conversion
						if (format == ImageFormat.NV21)// || format == ImageFormat.YUY2 || format == ImageFormat.NV16)
						{
							int w = parameters.getPreviewSize().width;
							int h = parameters.getPreviewSize().height;
							// Get the YuV image
							YuvImage yuv_image = new YuvImage(data, format, w, h, null);
							// Convert YuV to Jpeg
							//Rect rect = new Rect(0, 0, w, h);
							//ByteArrayOutputStream output_stream = new ByteArrayOutputStream();
							//yuv_image.compressToJpeg(rect, 100, output_stream);
							//byte[] byt=output_stream.toByteArray();
						}
					}
				});
			}
			catch (Throwable t)
			{
				Log.e("PreviewDemo-surfaceCallback",
					  "Exception in setPreviewDisplay()", t);
				Toast
					.makeText(MainActivity.this, t.getMessage(), Toast.LENGTH_LONG)
					.show();
			}

			if (!cameraConfigured)
			{
				Camera.Parameters parameters=camera.getParameters();
				Camera.Size size=getBestPreviewSize(width, height,
													parameters);

				if (size!=null)
				{
					parameters.setPreviewSize(size.width, size.height);
					camera.setParameters(parameters);
					cameraConfigured=true;
				}
			}
		}
	}

	private void startPreview() 
	{
		if (cameraConfigured && camera!=null)
		{
			camera.startPreview();
			inPreview=true;
		}
	}

	SurfaceHolder.Callback surfaceCallback=new SurfaceHolder.Callback()
	{
		public void surfaceCreated(SurfaceHolder holder)
		{
			// no-op -- wait until surfaceChanged()
		}

		public void surfaceChanged(SurfaceHolder holder,
								   int format, int width,
								   int height) 
		{
			initPreview(width, height);
			startPreview();
		}

		public void surfaceDestroyed(SurfaceHolder holder) 
		{
			// no-op
		}
	};

}

 */
