#include <iostream>
#include <math.h>
#include <mpi.h> ////
#include <stdio.h>////
#include <stdlib.h>
#include <string.h>
#include <msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}
void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

int main(int args, char** argv)
{
	MPI_Init(NULL, NULL);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size); //processors number
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //processor id
	int ImageWidth = 4, ImageHeight = 4; //imgwidth=cols ,,imgheight=rows
	int* imageData = NULL; //1d img data
	int processLenght,start, end , recvcount, subStart , remenderlength;
	int* subarr = NULL;//contain sub send data
	MPI_Status stut;
	if(rank == 0)
	{
		// start get img data
		System::String^ imagePath;
		std::string img;
		img = "..//Data//Input//test.png";     //img path
		imagePath = marshal_as<System::String^>(img);
		imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
		// end get img data
	    //start split the data	1darr
		processLenght = (ImageHeight / size) *ImageWidth; //need to be send to other process //kam height
		remenderlength = (ImageHeight % size) * ImageWidth;
		for (int i = 1;i < size;i++)
		{
			start = i * processLenght; //zero index 
			if (i == size - 1) processLenght += remenderlength;
			subStart = ImageWidth;
			MPI_Send(&processLenght, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&ImageHeight  , 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&ImageWidth   , 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&subStart, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			if (i == size - 1)
			{
				recvcount = processLenght + ImageWidth;
				MPI_Send(&recvcount         ,   1      , MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&imageData[start - ImageWidth],recvcount , MPI_INT, i, 0, MPI_COMM_WORLD);
			}
			else
			{
				recvcount = processLenght + ImageWidth*2;
				MPI_Send(&recvcount ,   1 , MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&imageData[start - ImageWidth], recvcount, MPI_INT, i,0, MPI_COMM_WORLD);
			}
		}
		//process 0 data
		processLenght = (ImageHeight / size) * ImageWidth;
		if(size>1)
			recvcount = processLenght + ImageWidth;
		else 
			recvcount = processLenght;
		subStart = 0;
		subarr = new int[recvcount];
		for (int i = 0;i < recvcount;i++)
			subarr[i] = imageData[i];
		//end split the data 1darr
	}
	else if(rank<size)  //start recv data 
	{
		MPI_Recv(&processLenght, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,&stut);
		MPI_Recv(&ImageHeight,   1, MPI_INT, 0, 0, MPI_COMM_WORLD,&stut);
		MPI_Recv(&ImageWidth,    1, MPI_INT, 0, 0, MPI_COMM_WORLD,&stut);
		MPI_Recv(&subStart,      1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stut);
		MPI_Recv(&recvcount,     1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stut);
		subarr =new int[recvcount];
		MPI_Recv(subarr, recvcount, MPI_INT, 0, 0, MPI_COMM_WORLD, &stut);
	} //end recv data
	//start process code
	int* subArrSum = new int[processLenght + remenderlength];//for answer data
	double localsum = 0;
	int positions[9], incIndex = 0;
	end = processLenght + ImageWidth;
	if (rank == 0)
		end = processLenght;
	for (int i = subStart; i < end; i++)
	{
		positions[4] = subarr[i];	//center point
		if (i - ImageWidth < 0) //upper center point
			positions[1] = subarr[i]; //mfe4 upper center
		else
			positions[1] = subarr[i - ImageWidth]; //fe upper center
		if (i + ImageWidth >= recvcount) //down center point
			positions[7] = subarr[i]; //mfe4 down center
		else
			positions[7] = subarr[i + ImageWidth]; //fe down center
		if (i % ImageWidth == 0)	//left point
		{
			positions[3] = positions[4]; //mfe4 left
			positions[0] = positions[4] ;//mfe4 upper left
			positions[6] = positions[7]; //mfe4 down left
		}
		else
		{
			positions[3] = subarr[i - 1]; //fe left
			if (i - 1 - ImageWidth < 0) //upper left point
				positions[0] = positions[4]; //mfe4 upper left
			else
				positions[0] = subarr[i - 1 - ImageWidth]; //fe upper left
			if (i - 1 + ImageWidth >= recvcount) //down left point
				positions[6] = subarr[i]; //mfe4 down left
			else
				positions[6] = subarr[i - 1 + ImageWidth]; //fe down left
		}
		if ((i+1) % ImageWidth == 0)	//right  point
		{
			positions[5] = positions[4]; //mfe4 right
			positions[2] = positions[4];//mfe4 upper right
			positions[8] = positions[7]; //mfe4 down right
		}
		else
		{
			positions[5] = subarr[i + 1]; //fe right
			if (i + 1 - ImageWidth < 0) //upper right point
				positions[2] = positions[4]; //mfe4 upper right
			else
				positions[2] = subarr[i + 1 - ImageWidth]; //fe upper right
			if (i + 1 + ImageWidth >= recvcount) //down right point
				positions[8] = subarr[i]; //mfe4 down right
			else
				positions[8] = subarr[i + 1 + ImageWidth]; //fe down right
		}
		for (int j = 0;j < 9;j++)      //divide by 9 try change it change the values and the photo
			localsum += positions[j] / 9;  
		subArrSum[incIndex++] = localsum ;
		localsum = 0;
	}
	if (rank >=1)//send answer data
	{
		MPI_Send(&processLenght, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(subarr,processLenght, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}
	if (rank == 0)//collect data
	{
		int inc=0;
		for (int i=0;i < processLenght;i++)
			imageData[inc++] = subArrSum[i];
		for (int j = 1;j < size;j++)
		{
			MPI_Recv(&processLenght, 1, MPI_INT, j, 0, MPI_COMM_WORLD, &stut);//rank j
			MPI_Recv(subArrSum, processLenght, MPI_INT, j, 0, MPI_COMM_WORLD, &stut);//rank j
			for (int i=0;i < processLenght;i++)
				imageData[inc++] = subArrSum[i];
		}
	//make img
		//start clock times
		int start_s, stop_s, TotalTime = 0;
		start_s = clock();
		createImage(imageData, ImageWidth, ImageHeight, 0);
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time: " << TotalTime << endl;
		free(imageData);
	}
	MPI_Finalize();
	return 0;
}

/*Width = 2000   Height = 1956
	cout << ImageWidth << "\t" << ImageHeight << endl;
	int x;
	for (int i = 0;i < ImageHeight * ImageWidth;i++)
	{
		cout << imageData[i];
		if ((i + 1) % ImageWidth == 0)
				cout << endl;
	}
	*/
	//cout << rank << "\t" << processLenght << "\t" << ImageHeight << "\t" << ImageWidth  << "\t" << recvcount<<"\t"<<
			//rank*processLenght<<"\t"<<(rank+1)*processLenght<< endl;
		//cout << rank << "\t" << subarr[subStart] << "\t" << subarr[subStart + processLenght -1] << endl;
