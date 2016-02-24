/*

Copyright (c) 2015, Stuart Mead - Risk Frontiers, Macquarie University
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
      
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
      
    * Neither the name of the copyright holder nor the names of its contributors
      may be used to endorse or promote products derived from this software without
      specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  For further information, contact:
    Stuart Mead
    Risk Frontiers
    Dept. of Environmental Sciences
    Macquarie University
    North Ryde NSW 2109
*/
// BEGIN PLUGIN:

// =============

// HEADER FILES:

#include <fstream> //std::ifstream
#include <iostream> // std::cout
#include <vector>
#include <string>

#include <maya/MIOStream.h>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgParser.h>
#include <maya/MObject.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MColorArray.h>
#include <maya/MFnPlugin.h>

#define kFilenameFlag       "-f"
#define kFilenameLongFlag       "-file"

#define VERSION "1.1.0"

// MAIN CLASS FOR THE MEL COMMAND:

class MArgList;

class plyImport : public MPxCommand

{

   public:

      plyImport();

      virtual ~plyImport();

      MStatus doIt( const MArgList& );
      MStatus redoIt();
      MStatus undoIt();
      bool isUndoable() const;


      static void* creator();
      static MSyntax newSyntax();
      MStatus parseArgs( const MArgList&);
      
      MStatus GetReferenceStringPointer(const MString&, void**, MString&);
      MStatus doImport();

   private:
      MString filename;
      
};





// CONSTRUCTOR:

plyImport::plyImport() : filename("")

{

}





// DESTRUCTOR CLEARS, AND FREES MEMORY OF THE PREVIOUS SELECTION LIST:

plyImport::~plyImport()

{
   //filename.clear();
}





// FOR CREATING AN INSTANCE OF THIS COMMAND:

void* plyImport::creator()

{

   return new plyImport;

}





// MAKE THIS COMMAND UNDOABLE:

bool plyImport::isUndoable() const

{

   return false;

}





// SAVE THE PREVIOUS SELECTION, AND THEN CALL redoIt(), WHICH DOES MOST OF THE WORK:

MStatus plyImport::doIt(const MArgList& args)

{
    //Implement the Mel plyImport command
    //args - argument list
    MStatus stat = MS::kSuccess;
    stat = parseArgs(args);
    if (stat != MS::kSuccess)
        return stat;

   return redoIt();

}

//Syntax

MSyntax plyImport::newSyntax() 
{
    MSyntax syn;

    syn.addFlag(kFilenameFlag, kFilenameLongFlag, MSyntax::kString);

    return syn;
}

//Parser
MStatus plyImport::parseArgs( const MArgList& args )
{
    MStatus status = MS::kSuccess;

    MArgDatabase parser(syntax(),args,&status);
    
    if(status != MS::kSuccess) {
    // arguments could not be parsed!
        return MS::kFailure;
    }
    if (parser.isFlagSet(kFilenameFlag))
    {
        parser.getFlagArgument(kFilenameFlag,0, filename);
    }
    else
    { 
        return MS::kFailure;
    }
    return status;
}

//Implement the import

MStatus plyImport::redoIt()

{
    return doImport();
}


MStatus plyImport::doImport()
{
    MStatus status = MS::kFailure;
    
    if (filename != "")
    {
        //Read the header
        std::ifstream ifs(filename.asChar(),ios::in);
        if (!ifs)
        {
            displayInfo("ERROR: Cannot open file");
            return status;
        }

        std::string str;
        ifs >>  str;
        if (str != "ply")
        {
            displayInfo("ERROR: First line is not ply");
            return status;
        }
        ifs >> str >> str >> str;//Format
        ifs >> str >> str >> str >> str;//Comment
        int numVertices;
        ifs >> str >> str >> numVertices;
        MString a;
        a +=(int)numVertices;
        displayInfo(a);
        ifs >> str >> str >> str;
        ifs >> str >> str >> str;
        ifs >> str >> str >> str;
        ifs >> str >> str >> str;
        ifs >> str >> str >> str;
        ifs >> str >> str >> str;
        ifs >> str >> str >> str;
        int numElems;
        ifs >> str >> str >> numElems;
        MString b;
        b +=(int)numElems;
        displayInfo(b);
        ifs >> str >> str >> str >> str >> str;//property list uchar int vertex_index
        
        ifs >> str ;
        if (str != "end_header")
        {
            displayInfo("ERROR: Last line is not end_header");
            displayInfo(str.c_str());
            return status;
        }

        ifs.close();
        
        //Setup arrays
        double (*pts4)[4] = new double[numVertices][4];
        double (*colors4)[4] = new double[numVertices][4];
        int * vertexIndices = new int[numVertices];
        //Now read binary data
        FILE * plyFile;
        plyFile = fopen(filename.asChar(), "rb+");
        if (plyFile == NULL)
        {
            displayInfo("ERROR: Cannot open file");
            return status;
        }
        char rubbish [80];
        //Skip the first 14 lines. WARNING THIS COULD BE VERY BAD
        for (int skip = 0; skip < 14; ++skip)
        {
            fgets(rubbish,40,plyFile);
        }
        
        MFloatPointArray vertexarray(numVertices);
        MIntArray vertexIndicesarray(numVertices);
        MColorArray vertexcolors(numVertices);
        for (int i = 0; i < numVertices; ++i)
        {
            float x,y,z;
            unsigned char r,g,b,a;
            fread(&x,sizeof(float),1,plyFile);
            fread(&y,sizeof(float),1,plyFile);
            fread(&z,sizeof(float),1,plyFile);
            fread(&r,sizeof(unsigned char),1,plyFile);
            fread(&g,sizeof(unsigned char),1,plyFile);
            fread(&b,sizeof(unsigned char),1,plyFile);
            fread(&a,sizeof(unsigned char),1,plyFile);
            vertexarray.set(i,x,y,z);
            float fr,fg,fb,fa;
            fr = r/(float)255;
            fg = g/(float)255;
            fb = b/(float)255;
            fa = a/(float)255;
            vertexcolors.set(i,fr,fg,fb,fa);
            vertexIndicesarray.set(i,i);
            if (i % 100000 == 0)
            {
                MString ind;
                ind +=(int)i;
                displayInfo(ind);
                MString readx;
                readx +=(double)x;
                displayInfo(readx);
                MString ready;
                ready +=(double)y;
                displayInfo(ready);
                MString readz;
                readz +=(double)z;
                displayInfo(readz);
                MString readr;
                readr +=(float)fr;
                displayInfo(readr);
                MString readg;
                readg +=(float)fg;
                displayInfo(readg);
                MString readb;
                readb +=(float)fb;
                displayInfo(readb);
                MString reada;
                reada +=(float)fa;
                displayInfo(reada);
            }
        }
        displayInfo("Read vertices");
        

        int * polyNFaces = new int[numElems];
        std::vector<int> polyIndices;
        //Now read faces
        for (int j = 0; j < numElems; ++j)
        {
            fread(&polyNFaces[j],sizeof(unsigned char),1,plyFile);
            for (int k = 0; k < polyNFaces[j]; ++k)
            {
                int l;
                fread(&l,1,sizeof(int),plyFile);
                polyIndices.push_back(l);
            }
        }
        //Add the indices to an array
        int * polyIndex = new int [polyIndices.size()];

        for (int iT = 0; iT < polyIndices.size(); ++iT)
        {
            polyIndex[iT] = polyIndices[iT];
        }
        MIntArray polyCountArray(polyNFaces,numElems);
        MIntArray polyConnectionArray(polyIndex,polyIndices.size());
        displayInfo("Read faces");
        //Add Mesh
        MFnMesh fn;
        MObject obj = fn.create(numVertices,
                                    numElems,
                                    vertexarray,
                                    polyCountArray,
                                    polyConnectionArray,
                                    MObject::kNullObj,
                                    &status);
        

        status = fn.setVertexColors(vertexcolors,vertexIndicesarray,(MDGModifier*)NULL);
        displayInfo("Added colors");
        //Cleanup
        delete [] colors4;
        delete [] vertexIndices;
        delete [] pts4;
        delete [] polyNFaces;
        delete [] polyIndex;
        fclose(plyFile);
    }
        setResult(status);
        return status;

}


// TO UNDO THE COMMAND, SIMPLY RESTORE THE ORIGINAL SELECTION BEFORE THE COMMAND WAS INVOKED:

MStatus plyImport::undoIt()

{

   return MS::kSuccess;

}





// INITIALIZES THE PLUGIN:

MStatus initializePlugin(MObject obj)

{

   MStatus status;

   MFnPlugin plugin(obj, "RiskFrontiers", "4.0", "Any");



   status = plugin.registerCommand("importPly", plyImport::creator, plyImport::newSyntax);

   if (!status)

   {

      status.perror("registerCommand");

      return status;

   }



   return status;

}





// UNINITIALIZES THE PLUGIN:

MStatus uninitializePlugin(MObject obj)

{

   MStatus status;

   MFnPlugin plugin(obj);



   status = plugin.deregisterCommand("importPly");

   if (!status)

   {

      status.perror("deregisterCommand");

      return status;

   }



   return status;

}





// ============================================================================================================== END PLUGIN.

