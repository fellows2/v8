
INT_SIZE = 4;

	//setting up pixel dimensions
	width = 640;
	height = 480;
	numElements = height * width;
	numPixels = 4 * numElements;

function main(){
	var seed = 1;


	//var t_I = runJS(seed);
	var h_I = runCuda(seed);

	//temp check to see if things seem reasonable
	/*
	for(i = 0; i < numPixels; i++){
		print(t_I[i]);
	}
	*/

	//testResult(t_I, t_I);
}

function runCuda(seed){
	var blocks, threads;

	//Creating host memory for pixel array
	print("creating host memory");
	var h_I = new Int32Array(numPixels); 

	//Retrieving Device
	print("retrieving Device Info");
	var dev = webcuda.Device(0);

	//Setting up Context for CUDA
	print("creating Context");
	var ctx = webcuda.newContext(0, dev);

	//Creating device memory for pixel array
	print("allocating CUDA memory");
	var d_I = webcuda.memAlloc(h_I.buffer);


	
	//Loading Module
	print("loading CUDA module");
	var module = webcuda.moduleLoad("fake");

	//Retrieving Function from Module
	var cuFunc = wecuda.getFunction(module, "rng");

	//Launching the Kernel
	webcuda.launchKernel(cuFunc, [40,30,0], [16,16,0], 
			[{
				type: "DevicePtr",
				value: d_I.devicePtr()
			}]);


	//Retrieving Data from CUDA Device Memory
	
	print("copying CUDA Mem Result to device");
	webcuda.copyDtoH(h_I, d_I);

	//temp check to see if things seem reasonable
	print("checking results");
	var value = 0;
	for(i = 0; i < numPixels; i++){
		value += h_I[i];
		//print(h_I[i]);
	}



	//Freeing CUDA Memory
	print("freeing CUDA memory");
	webcuda.free(d_I);

	//Freeing CUDA context
	//NEED TO IMPLEMENT THIS!!!
	//ctx.destroy();

}

function runJS(seed){
	//creating array for testing in javascript to verify results
	var t_I = new Int32Array(numPixels);

	var delta = 0x9E3779B9;
	var k0 = 0xA341316C;
	var k1 = 0xC8013EA4;
	var k2 = 0xAD90777D;
	var k3 = 0x7E95761E;
	var ITER = 15;

	var i,j;
	for(i = 0; i < numElements; i++){

		var x = seed;
		var y = seed << 3;

		x += i + (i << 11) + (i << 19);
		y += i + (i << 9) + (i << 21);    

		var sum = 0;
		for (j=0; j < ITER; j++) {
			sum += delta;
			x += ((y << 4) + k0) & (y + sum) & ((y >> 5) + k1);
			y += ((x << 4) + k2) & (x + sum) & ((x >> 5) + k3);
		}

		var r = x & 0xFF;
		var g = (x & 0xFF00) >> 8;

		t_I[i*4  ] = r;
		t_I[i*4+1] = r;
		t_I[i*4+2] = r;
		t_I[i*4+3] = g;

	}

	return t_I;

}

function testResult(h_I, t_I){
	var i;
	for (i = 0; i < numElements*4; i++)
	{
		if (Math.abs(t_I[i] - h_I[i]) > 1e-5)
		{
			print("Result verification failed at element " + i);
			print("Host element value: " + h_I[i] + ", CUDA element value: " + t_I[i]);
		}
	}
	print("Test PASSED\n");
}

main();
