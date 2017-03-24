/*=========================================================================

  Program:   OpenIGTLink -- Example for Data Receiving Server Program
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <iostream>
#include <iomanip>
#include <math.h>
#include <cstdlib>
#include <cstring>

#include "igtlOSUtil.h"
#include "igtlMessageHeader.h"
#include "igtlTransformMessage.h"
#include "igtlImageMessage.h"
#include "igtlServerSocket.h"
#include "igtlStatusMessage.h"
#include "igtlPositionMessage.h"

#if OpenIGTLink_PROTOCOL_VERSION >= 2
#include "igtlPointMessage.h"
#include "igtlTrajectoryMessage.h"
#include "igtlStringMessage.h"
#include "igtlBindMessage.h"
#include "igtlCapabilityMessage.h"
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2



#if OpenIGTLink_PROTOCOL_VERSION >= 2
int ReceivePointAndSendBack(igtl::Socket * socket, igtl::MessageHeader * header);
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 2) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << " <port>"    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    exit(0);
    }

  int    port     = atoi(argv[1]);

  igtl::ServerSocket::Pointer serverSocket;
  serverSocket = igtl::ServerSocket::New();
  int r = serverSocket->CreateServer(port);

  if (r < 0)
    {
    std::cerr << "Cannot create a server socket." << std::endl;
    exit(0);
    }

  igtl::Socket::Pointer socket;
  
  while (1)
    {
    //------------------------------------------------------------
    // Waiting for Connection
    socket = serverSocket->WaitForConnection(1000);
    
    if (socket.IsNotNull()) // if client connected
      {
      // Create a message buffer to receive header
      igtl::MessageHeader::Pointer headerMsg;
      headerMsg = igtl::MessageHeader::New();

      //------------------------------------------------------------
      // Allocate a time stamp 
      igtl::TimeStamp::Pointer ts;
      ts = igtl::TimeStamp::New();

      //------------------------------------------------------------
      // loop
      for (int i = 0; i < 100; i ++)
        {

        // Initialize receive buffer
        headerMsg->InitPack();

        // Receive generic header from the socket
        int r = socket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
        if (r == 0)
          {
          socket->CloseSocket();
          }
        if (r != headerMsg->GetPackSize())
          {
          continue;
          }

        // Deserialize the header
        headerMsg->Unpack();

        // Get time stamp
        igtlUint32 sec;
        igtlUint32 nanosec;
        
        headerMsg->GetTimeStamp(ts);
        ts->GetTimeStamp(&sec, &nanosec);

        std::cerr << "Time stamp: "
                  << sec << "." << std::setw(9) << std::setfill('0') 
                  << nanosec << std::endl;

        // Check data type and receive data body
       
#if OpenIGTLink_PROTOCOL_VERSION >= 2
         if (strcmp(headerMsg->GetDeviceType(), "POINT") == 0)
          {
	      ReceivePointAndSendBack(socket, headerMsg);
          }
        
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2
        else
          {
          // if the data type is unknown, skip reading.
          std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
          std::cerr << "Size : " << headerMsg->GetBodySizeToRead() << std::endl;
          socket->Skip(headerMsg->GetBodySizeToRead(), 0);
          }
        }
      }
    }
    
  //------------------------------------------------------------
  // Close connection (The example code never reaches to this section ...)
  
  socket->CloseSocket();

}







#if OpenIGTLink_PROTOCOL_VERSION >= 2

int ReceivePointAndSendBack(igtl::Socket * socket, igtl::MessageHeader * header)
{

	std::cerr << "Receiving POINT data type." << std::endl;

	// Create a message buffer to receive transform data
	igtl::PointMessage::Pointer pointMsg;
	pointMsg = igtl::PointMessage::New();
	pointMsg->SetMessageHeader(header);
	pointMsg->AllocatePack();

	// Receive transform data from the socket
	socket->Receive(pointMsg->GetPackBodyPointer(), pointMsg->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = pointMsg->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		int nElements = pointMsg->GetNumberOfPointElement();
		for (int i = 0; i < nElements; i++)
		{
			igtl::PointElement::Pointer pointElement;
			pointMsg->GetPointElement(i, pointElement);

			igtlUint8 rgba[4];
			pointElement->GetRGBA(rgba);

			igtlFloat32 pos[3];
			pointElement->GetPosition(pos);


			igtl::PointMessage::Pointer pointMsgRcv;
			pointMsgRcv = igtl::PointMessage::New();
			pointMsgRcv->SetDeviceName("PointSendingBack");

			igtl::PointElement::Pointer point0;
			point0 = igtl::PointElement::New();
			point0->SetName(pointElement->GetName());
			point0->SetGroupName(pointElement->GetGroupName());
			point0->SetRGBA((int)rgba[0], (int)rgba[1], (int)rgba[2], (int)rgba[3]);
			point0->SetPosition(-pos[0], -pos[1], -pos[2]);
			point0->SetRadius(pointElement->GetRadius());
			point0->SetOwner(pointElement->GetOwner());

			pointMsgRcv->AddPointElement(point0);
			pointMsgRcv->Pack();

			igtl::Sleep(500);
			socket->Send(pointMsgRcv->GetPackPointer(), pointMsgRcv->GetPackSize());


			std::cerr << "========== Element #" << i << " ==========" << std::endl;
			std::cerr << " Name      : " << pointElement->GetName() << std::endl;
			std::cerr << " GroupName : " << pointElement->GetGroupName() << std::endl;
			std::cerr << " RGBA      : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
			std::cerr << " Position  : ( " << std::fixed << pos[0] << ", " << pos[1] << ", " << pos[2] << " )" << std::endl;
			std::cerr << " Radius    : " << std::fixed << pointElement->GetRadius() << std::endl;
			std::cerr << " Owner     : " << pointElement->GetOwner() << std::endl;
			std::cerr << "================================" << std::endl;
		}
	}

	return 1;
}



#endif //OpenIGTLink_PROTOCOL_VERSION >= 2
