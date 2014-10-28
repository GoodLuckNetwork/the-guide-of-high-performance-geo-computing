#include "gt_geometry.h"
#include "gt_spatialindex.h"
#include "gt_datasource.h"
#include  "gt_datadriver.h"
#include "ogrsf_frmts.h"
#include "ogr_feature.h"
#include "gt_spatialreference.h"
#include "ogr_geometry.h"
#include "mpi.h"
#include <iostream>

using namespace std;
int main( int nArgc, char *papszArgv[] )
{	
	OGRRegisterAll();
	//****************************//
	//*******��������********//	
	//****************************//
	char *pszSrcFilename=NULL;
	char *layername = NULL;
	char *newlayer = NULL;
	const char  *pszsrsputSRSDef = NULL;
	const char  *pszOutputSRSDef = NULL;
	OGRSpatialReference *ogrsrsputSRS = (OGRSpatialReference*)OSRNewSpatialReference(NULL);	//�洢ԭͶӰ
	OGRSpatialReference *ogrOutputSRS = (OGRSpatialReference*)OSRNewSpatialReference(NULL);	//�洢Ŀ��ͶӰ	
	gts::GTGDOSMySQLDataSource *pSrc= new GTGDOSMySQLDataSource();
	char* oHost=NULL,*oPassword=NULL,*oUser=NULL,*oDB=NULL, **papszTableNames=NULL;
	int nPort=0,j=0;
	/*papszArgv[0]="hpgc_defineprj";
	papszArgv[1]="-l";	
	papszArgv[2]="new_point6";
	papszArgv[3]="-nl";
	papszArgv[4]="newpoint11";
	papszArgv[5]="-t_srs";
	papszArgv[6]="EPSG:4326";
	papszArgv[7]="MYSQL:testing_mysql2,user=root,password=123456,host=localhost,port=3306";
	nArgc=8;*/

	nArgc = OGRGeneralCmdLineProcessor( nArgc, &papszArgv, 0 );
	if( nArgc < 1 )
	{
		MPI_Finalize();
        return 0;
	}

	for( int iArg = 1; iArg < nArgc; iArg++ )
	{
		if( EQUAL(papszArgv[iArg], "-l") )
		{
			layername = papszArgv[++iArg];
			if (layername==NULL)
			{
				printf("Get source layer name failed");
				
				return 0 ;
			}	

		}
		else if( EQUAL(papszArgv[iArg], "-nl") )
		{
			newlayer = papszArgv[++iArg];
			if (newlayer==NULL)
			{
				printf("Get target layer name failed");
				
				return 0 ;
			}	

		}
		else if ( EQUAL(papszArgv[iArg], "-a_srs") )
		{
			pszsrsputSRSDef = papszArgv[++iArg];
			ogrsrsputSRS->SetFromUserInput(pszsrsputSRSDef );
			if (ogrsrsputSRS==NULL)
			{
				printf("Get source srs failed");
				
				return 0;
			}			
		}

		else if ( EQUAL(papszArgv[iArg], "-t_srs") )
		{
			pszOutputSRSDef = papszArgv[++iArg];
			ogrOutputSRS->SetFromUserInput( pszOutputSRSDef );			
			if (ogrOutputSRS==NULL)
			{
				printf("Get target srs failed");
				
				return 0;
			}			
		}

		else if( pszSrcFilename==NULL)
		{
			pszSrcFilename = papszArgv[iArg];
			if (pszSrcFilename==NULL)
			{
				printf("Get src failed");
				
				return 0;
			}						
		}
	}
	
	
	//****************************//
	//*******���ݿ��������********//	
	//****************************//
	string pstr(pszSrcFilename);
	if(EQUAL(pstr.substr(0,6).c_str(),"MYSQL:"))
	{//try to get the srs of a gdao-mysql-datasource
				
		char **papszItems = CSLTokenizeString2( pszSrcFilename+6, ",", CSLT_HONOURSTRINGS );
		if( CSLCount(papszItems) < 1 )
		{
			CSLDestroy( papszItems );
			CPLError( CE_Failure, CPLE_AppDefined, "MYSQL: request missing databasename." );
			
			return 0;
		}
		oDB=papszItems[0];
		for( j = 1; papszItems[j] != NULL; j++ )
		{
			if( EQUALN(papszItems[j],"user=",5) )
				oUser = papszItems[j] + 5;
			else if( EQUALN(papszItems[j],"password=",9) )
				oPassword = papszItems[j] + 9;
			else if( EQUALN(papszItems[j],"host=",5) )
				oHost = papszItems[j] + 5;
			else if( EQUALN(papszItems[j],"port=",5) )
				nPort = atoi(papszItems[j] + 5);
			else if( EQUALN(papszItems[j],"tables=",7) )
			{
				papszTableNames = CSLTokenizeStringComplex( 
					papszItems[j] + 7, ";", FALSE, FALSE );
			}
			else
				CPLError( CE_Warning, CPLE_AppDefined, 
							"'%s' in MYSQL datasource definition not recognised and ignored.", papszItems[j] );					

		}
	}	

	//****************************//
	//*******������Դ*********//	
	//****************************//
	
	
	
	bool isOpen = pSrc->openDataSource("Mysql",oUser,oPassword,oHost,nPort,oDB);//���Ѵ洢ԭ���ݵ����ݿ�
	if (!isOpen)//�ж��Ƿ������Դ���ݿ�
	{
		printf( "open datasource failed..\n");
		GTGDOSMySQLDataSource::destroyDataSource(pSrc);
		return 0;
	}	
	
	GTFeatureLayer *pSrcLyr = pSrc->getFeatureLayerByName(layername, false);//�����ݿ���ͨ��ͼ����ȡ��ͼ�㣬����Ϊpoint,false=update
	if (!pSrcLyr)
	{
		printf( "open layer failed..");
		GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
		GTGDOSMySQLDataSource::destroyDataSource(pSrc);
		return 0;
	}
	//****************************//
	//*******��ȡ�ռ�ο�*********//	
	//****************************//
	GTSpatialReference *poSourceSRS = new GTSpatialReference();
	

    poSourceSRS = pSrcLyr->getLayerSpatialRefPtr();//��ȡԴͶӰ�ռ�ο������룩        			
	
	//****************************//
	//*******����OGR�ռ�ο�*********//	
	//****************************//

	OGRCoordinateTransformation *poCT = NULL;		
	OGRSpatialReference *ogrSourceSRS =(OGRSpatialReference*)OSRNewSpatialReference(NULL);	
	ogrSourceSRS=poSourceSRS->getOGRSpatialRefPtr();//Get sourceSRS

	if (pszsrsputSRSDef!=NULL && ogrSourceSRS!=NULL)
	{
		printf("source data already had projection,pleace not define new projection.\n" );
		GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
		GTGDOSMySQLDataSource::destroyDataSource(pSrc);
		return 0;
	}

	if(ogrSourceSRS==NULL)
	{
		printf("Get sourceSRS failed\n");			
		GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
		GTGDOSMySQLDataSource::destroyDataSource(pSrc);
		return 0;
	}
	//Get outputSRS
	if(ogrOutputSRS==NULL)
	{
		printf("Get OutputSRS failed\n");
		GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
		GTGDOSMySQLDataSource::destroyDataSource(pSrc);
		return 0;
	}
	if(pszsrsputSRSDef!=NULL)
		poCT = OGRCreateCoordinateTransformation( ogrsrsputSRS, ogrOutputSRS );//Create Coordinate Transformation
	else
		poCT = OGRCreateCoordinateTransformation( ogrSourceSRS, ogrOutputSRS );
	if (poCT == NULL)
	{
		printf("Create Coordinate Transformation failed\n");
		GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
		GTGDOSMySQLDataSource::destroyDataSource(pSrc);
		return 0;
	}

	//*******����ת������*********//
	char**      papszTransformOptions = NULL;
	 if (poCT != NULL && ogrOutputSRS->IsGeographic())
    {
        papszTransformOptions =
            CSLAddString(papszTransformOptions, "WRAPDATELINE=YES");
    }
    else if (poSourceSRS != NULL && ogrOutputSRS == NULL && ogrSourceSRS->IsGeographic())
    {
        papszTransformOptions =
            CSLAddString(papszTransformOptions, "WRAPDATELINE=YES");
    }
    else
    {
       // fprintf(stderr, "-wrapdateline option only works when reprojecting to a geographic SRS\n");
    }
	//printf("��ȡת��������\n");

	//****************************//
	//****Create parallel mpi*****//	
	//****************************//  
	int mpi_rank, mpi_size;
	
	MPI_Init(&nArgc, &papszArgv);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	
	//****************************//
	//*******�������ͼ��*********//	
	//****************************//
	pSrc->deleteFeatureLayerByName(newlayer);//delete haved layer
	GTFeatureLayer *pDrcLyr=new GTFeatureLayer();
	if(mpi_rank==0)
	{
		pDrcLyr = pSrc->createFeatureLayer(newlayer,NULL,pSrcLyr->getFieldsDefnPtr(),pSrcLyr->getGeometryType());	//����������ݲ�
		char **srsinfo = NULL;
		srsinfo=(char**)malloc(1000*sizeof(char));
			for (int i = 0;i< 1000 ;i++ )
		{
			*srsinfo = (char*) malloc(sizeof(char)*1000);
		}
		ogrOutputSRS->exportToWkt(srsinfo);			
		pSrc->defineLayerSpatialReference(newlayer,*srsinfo);		
		if (!pDrcLyr)
		{
			printf( "Create layer failed..");
			
			GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
			GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
			GTGDOSMySQLDataSource::destroyDataSource(pSrc);
			MPI_Finalize();
			return 0;
		}
	
		//****************************//
		//*******����ͶӰת��*********//	
		//****************************//       
		//д����������	
		long N_count = pSrcLyr->getFieldsDefnPtr()->getFieldCount();
	
		for (long f = 0; f < N_count; f++)
		{
			bool isFail = pDrcLyr->createField(pSrcLyr->getFieldsDefnPtr()->getFieldPtr(f));
			if(isFail)
			{
				printf("Create Field failed\n");
				GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
				GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
				GTGDOSMySQLDataSource::destroyDataSource(pSrc);
				MPI_Finalize();
				return 0;
			}
		}
		//GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
	}
	GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
	MPI_Barrier(MPI_COMM_WORLD);
	pDrcLyr = pSrc->getFeatureLayerByName(newlayer,true);
	if (!pDrcLyr)
	{
		printf( "get new layer failed..");
		GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
		MPI_Finalize();
		return 0;
	}
	//printf("create attribute table successfully.\n");
	

	//д��ͼ������
	long feature_count = pSrcLyr->getFeatureCount();//get all feature count
	long feature_N = 0;//define every rank uses feature count
	long feature_S = 0;//start feature NUM
	long feature_E = 0;//end feature NUM
	if (mpi_size > feature_count && feature_count>0)//mpi's processers more than feature count
	{
		feature_S = mpi_rank;
		feature_E = mpi_rank+1;
	}
	else if(mpi_size < feature_count)
	{
		feature_N = feature_count/mpi_size;
		feature_S = mpi_rank*feature_N;

		if(mpi_rank == mpi_size-1)
		feature_E = feature_count;
		else
			feature_E = feature_S+feature_N;
	}
	double start_time = 0,end_time = 0;
	start_time = MPI_Wtime();
	for (feature_S; feature_S < feature_E; ++feature_S)//use while .......
	{
		GTFeature *add_Feat = pSrcLyr->getFeature(feature_S);//����ԭfeature����		
		GTGeometry *SrsGeometry = add_Feat->getGeometryPtr()->clone();
		
		/* �����������˳��,WKBXDR = 0,MSB/Sun/Motoroloa: Most Significant Byte First big-endian
		WKBNDR = 1,LSB/Intel/Vax: Least Significant Byte First little-endian*/			
		enumGTWKBByteOrder byteOrder = WKBNDR;
		OGRwkbByteOrder byteOrder2 = wkbNDR;

		/********�ж������������*********/	
		enumGTWKBGeometryType p_type=SrsGeometry->getGeometryWKBType();//��ȡ��������

		/********��������ΪPoint*********/	
		if( p_type==1)
		{
			OGRPoint* DestGeom = new OGRPoint();
			GTPoint * Destgeometry=new GTPoint();

			int nsize = SrsGeometry->getWKBSize();//��ȡͼ���ֽ�
			unsigned char* Datain =(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			unsigned char* Dataout=(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			SrsGeometry->exportToWkb(byteOrder,Datain);	//gt->wkb
			DestGeom->importFromWkb(Datain);//wkb->ogr
			if(DestGeom==NULL)
			{
				printf("no geom\n");
			}

			DestGeom->assignSpatialReference(ogrSourceSRS);//give geometry to assign a spatialreference

			if(poCT != NULL || papszTransformOptions != NULL)
			{
				//OGRGeometry* poReprojectedGeom = OGRGeometryFactory::transformWithOptions(DestGeom, poCT, papszTransformOptions);//ͶӰת��
				
				//printf("Before:%f\n",DestGeom->getX());
				bool isFalse = DestGeom->transform(poCT);//ͶӰת��			
				//printf("After:%f\n",DestGeom->getX());		
				if( isFalse )
				 {
					fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", feature_S );
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				 }			
				DestGeom->exportToWkb(byteOrder2,Dataout);//ogr->wkb
				Destgeometry->importFromWkb(Dataout);//wkb->gt		
				add_Feat->setGeometryDirectly(Destgeometry);			
				bool isSuccess = pDrcLyr->createFeature(add_Feat);
				if(!isSuccess)
				{
					printf("create feature failed.\n");
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				}
		
					//GTFeature::destroyFeature(Dest_feature);
					//GTFeature::destroyFeature(add_Feat);
				
			}
			delete []Datain;
			delete []Dataout;		
		}
		/********��������ΪLineString*********/	
		else if( p_type==2)
		{
			OGRLineString* DestGeom = new OGRLineString();
			GTLineString * Destgeometry=new GTLineString();

			int nsize = SrsGeometry->getWKBSize();//��ȡͼ���ֽ�
			unsigned char* Datain =(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			unsigned char* Dataout=(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			SrsGeometry->exportToWkb(byteOrder,Datain);	//gt->wkb
			DestGeom->importFromWkb(Datain);//wkb->ogr
			if(DestGeom==NULL)
			{
				printf("no geom\n");
			}

			DestGeom->assignSpatialReference(ogrSourceSRS);//give geometry to assign a spatialreference

			if(poCT != NULL || papszTransformOptions != NULL)
			{
				//OGRGeometry* poReprojectedGeom = OGRGeometryFactory::transformWithOptions(DestGeom, poCT, papszTransformOptions);//ͶӰת��
				
				//printf("Before:%f\n",DestGeom->getX());
				bool isFalse = DestGeom->transform(poCT);//ͶӰת��			
				//printf("After:%f\n",DestGeom->getX());		
				if( isFalse )
				 {
					fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", feature_S );
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				 }			
				DestGeom->exportToWkb(byteOrder2,Dataout);//ogr->wkb
				Destgeometry->importFromWkb(Dataout);//wkb->gt		
				add_Feat->setGeometryDirectly(Destgeometry);			
				bool isSuccess = pDrcLyr->createFeature(add_Feat);
				if(!isSuccess)
				{
					printf("create feature failed.\n");
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				}
		
					//GTFeature::destroyFeature(Dest_feature);
					//GTFeature::destroyFeature(add_Feat);
				
			}
			delete []Datain;
			delete []Dataout;		
		}
		/********��������ΪPolygon*********/	
		else if( p_type==3)
		{
			OGRPolygon* DestGeom = new OGRPolygon();
			GTPolygon * Destgeometry=new GTPolygon();

			int nsize = SrsGeometry->getWKBSize();//��ȡͼ���ֽ�
			unsigned char* Datain =(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			unsigned char* Dataout=(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			SrsGeometry->exportToWkb(byteOrder,Datain);	//gt->wkb
			DestGeom->importFromWkb(Datain);//wkb->ogr
			if(DestGeom==NULL)
			{
				printf("no geom\n");
			}

			DestGeom->assignSpatialReference(ogrSourceSRS);//give geometry to assign a spatialreference

			if(poCT != NULL || papszTransformOptions != NULL)
			{
				//OGRGeometry* poReprojectedGeom = OGRGeometryFactory::transformWithOptions(DestGeom, poCT, papszTransformOptions);//ͶӰת��
				
				//printf("Before:%f\n",DestGeom->getX());
				bool isFalse = DestGeom->transform(poCT);//ͶӰת��			
				//printf("After:%f\n",DestGeom->getX());		
				if( isFalse )
				 {
					fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", feature_S );
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				 }			
				DestGeom->exportToWkb(byteOrder2,Dataout);//ogr->wkb
				Destgeometry->importFromWkb(Dataout);//wkb->gt		
				add_Feat->setGeometryDirectly(Destgeometry);			
				bool isSuccess = pDrcLyr->createFeature(add_Feat);
				if(!isSuccess)
				{
					printf("create feature failed.\n");
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				}
		
					//GTFeature::destroyFeature(Dest_feature);
					//GTFeature::destroyFeature(add_Feat);
				
			}
			delete []Datain;
			delete []Dataout;		
		}
		/********��������ΪMultiPoint*********/	
		else if( p_type==4)
		{
			OGRMultiPoint* DestGeom = new OGRMultiPoint();
			GTMultiPoint * Destgeometry=new GTMultiPoint();

			int nsize = SrsGeometry->getWKBSize();//��ȡͼ���ֽ�
			unsigned char* Datain =(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			unsigned char* Dataout=(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			SrsGeometry->exportToWkb(byteOrder,Datain);	//gt->wkb
			DestGeom->importFromWkb(Datain);//wkb->ogr
			if(DestGeom==NULL)
			{
				printf("no geom\n");
			}

			DestGeom->assignSpatialReference(ogrSourceSRS);//give geometry to assign a spatialreference

			if(poCT != NULL || papszTransformOptions != NULL)
			{
				//OGRGeometry* poReprojectedGeom = OGRGeometryFactory::transformWithOptions(DestGeom, poCT, papszTransformOptions);//ͶӰת��
				
				//printf("Before:%f\n",DestGeom->getX());
				bool isFalse = DestGeom->transform(poCT);//ͶӰת��			
				//printf("After:%f\n",DestGeom->getX());		
				if( isFalse )
				 {
					fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", feature_S );
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				 }			
				DestGeom->exportToWkb(byteOrder2,Dataout);//ogr->wkb
				Destgeometry->importFromWkb(Dataout);//wkb->gt		
				add_Feat->setGeometryDirectly(Destgeometry);			
				bool isSuccess = pDrcLyr->createFeature(add_Feat);
				if(!isSuccess)
				{
					printf("create feature failed.\n");
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				}
		
					//GTFeature::destroyFeature(Dest_feature);
					//GTFeature::destroyFeature(add_Feat);
				
			}
			delete []Datain;
			delete []Dataout;		
		}

		/********��������ΪMultiLineString*********/	
		else if( p_type==5)
		{
			OGRMultiLineString* DestGeom = new OGRMultiLineString();
			GTMultiLineString * Destgeometry=new GTMultiLineString();

			int nsize = SrsGeometry->getWKBSize();//��ȡͼ���ֽ�
			unsigned char* Datain =(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			unsigned char* Dataout=(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			SrsGeometry->exportToWkb(byteOrder,Datain);	//gt->wkb
			DestGeom->importFromWkb(Datain);//wkb->ogr
			if(DestGeom==NULL)
			{
				printf("no geom\n");
			}

			DestGeom->assignSpatialReference(ogrSourceSRS);//give geometry to assign a spatialreference

			if(poCT != NULL || papszTransformOptions != NULL)
			{
				//OGRGeometry* poReprojectedGeom = OGRGeometryFactory::transformWithOptions(DestGeom, poCT, papszTransformOptions);//ͶӰת��
				
				//printf("Before:%f\n",DestGeom->getX());
				bool isFalse = DestGeom->transform(poCT);//ͶӰת��			
				//printf("After:%f\n",DestGeom->getX());		
				if( isFalse )
				 {
					fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", feature_S );
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				 }			
				DestGeom->exportToWkb(byteOrder2,Dataout);//ogr->wkb
				Destgeometry->importFromWkb(Dataout);//wkb->gt		
				add_Feat->setGeometryDirectly(Destgeometry);			
				bool isSuccess = pDrcLyr->createFeature(add_Feat);
				if(!isSuccess)
				{
					printf("create feature failed.\n");
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				}
		
					//GTFeature::destroyFeature(Dest_feature);
					//GTFeature::destroyFeature(add_Feat);
				
			}
			delete []Datain;
			delete []Dataout;		
		}

		/********��������ΪMultiPolygon*********/	
		else if( p_type==6)
		{
			OGRMultiPolygon* DestGeom = new OGRMultiPolygon();
			GTMultiPolygon * Destgeometry=new GTMultiPolygon();

			int nsize = SrsGeometry->getWKBSize();//��ȡͼ���ֽ�
			unsigned char* Datain =(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			unsigned char* Dataout=(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			SrsGeometry->exportToWkb(byteOrder,Datain);	//gt->wkb
			DestGeom->importFromWkb(Datain);//wkb->ogr
			if(DestGeom==NULL)
			{
				printf("no geom\n");
			}

			DestGeom->assignSpatialReference(ogrSourceSRS);//give geometry to assign a spatialreference

			if(poCT != NULL || papszTransformOptions != NULL)
			{
				//OGRGeometry* poReprojectedGeom = OGRGeometryFactory::transformWithOptions(DestGeom, poCT, papszTransformOptions);//ͶӰת��
				
				//printf("Before:%f\n",DestGeom->getX());
				bool isFalse = DestGeom->transform(poCT);//ͶӰת��			
				//printf("After:%f\n",DestGeom->getX());		
				if( isFalse )
				 {
					fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", feature_S );
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				 }			
				DestGeom->exportToWkb(byteOrder2,Dataout);//ogr->wkb
				Destgeometry->importFromWkb(Dataout);//wkb->gt		
				add_Feat->setGeometryDirectly(Destgeometry);			
				bool isSuccess = pDrcLyr->createFeature(add_Feat);
				if(!isSuccess)
				{
					printf("create feature failed.\n");
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				}
		
					//GTFeature::destroyFeature(Dest_feature);
					//GTFeature::destroyFeature(add_Feat);
				
			}
			delete []Datain;
			delete []Dataout;		
		}

		/********��������ΪGeometryCollection*********/	
		else if( p_type==7)
		{
			OGRGeometryCollection* DestGeom = new OGRGeometryCollection();
			GTGeometryBag * Destgeometry=new GTGeometryBag();

			int nsize = SrsGeometry->getWKBSize();//��ȡͼ���ֽ�
			unsigned char* Datain =(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			unsigned char* Dataout=(unsigned char*)malloc(nsize);//Ԥ���忪�ٿռ�
			SrsGeometry->exportToWkb(byteOrder,Datain);	//gt->wkb
			DestGeom->importFromWkb(Datain);//wkb->ogr
			if(DestGeom==NULL)
			{
				printf("no geom\n");
			}

			DestGeom->assignSpatialReference(ogrSourceSRS);//give geometry to assign a spatialreference

			if(poCT != NULL || papszTransformOptions != NULL)
			{
				//OGRGeometry* poReprojectedGeom = OGRGeometryFactory::transformWithOptions(DestGeom, poCT, papszTransformOptions);//ͶӰת��
				
				//printf("Before:%f\n",DestGeom->getX());
				bool isFalse = DestGeom->transform(poCT);//ͶӰת��			
				//printf("After:%f\n",DestGeom->getX());		
				if( isFalse )
				 {
					fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", feature_S );
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				 }			
				DestGeom->exportToWkb(byteOrder2,Dataout);//ogr->wkb
				Destgeometry->importFromWkb(Dataout);//wkb->gt		
				add_Feat->setGeometryDirectly(Destgeometry);			
				bool isSuccess = pDrcLyr->createFeature(add_Feat);
				if(!isSuccess)
				{
					printf("create feature failed.\n");
					GTFeature::destroyFeature(add_Feat);
					GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
					GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
					GTGDOSMySQLDataSource::destroyDataSource(pSrc);
					delete []Datain;
					delete []Dataout;
					MPI_Finalize();
					return 0;
				}
		
					//GTFeature::destroyFeature(Dest_feature);
					//GTFeature::destroyFeature(add_Feat);
				
			}
			delete []Datain;
			delete []Dataout;		
		}
		else
			printf("Other data type,not support.\n");
	/********������������*********/
	/*********�д�����***********/
	/********������������*********/


	}//ѭ��ĩ��
	MPI_Barrier(MPI_COMM_WORLD);
	end_time = MPI_Wtime();
	double t = end_time - start_time;
	if(mpi_rank==0)
	{
		printf("Time = %f \n",t);
		printf("start_time=%f\n",start_time);
		printf("end_time=%f\n",end_time);
	}

	//printf("create feature successfully.\n");
	GTFeatureLayer::destroyFeatureLayer(pSrcLyr);
	GTFeatureLayer::destroyFeatureLayer(pDrcLyr);
	CSLDestroy(papszTransformOptions);
	pSrc->closeDataSource();
	GTGDOSMySQLDataSource::destroyDataSource(pSrc);
	MPI_Finalize();
	return 0;
}
	
	