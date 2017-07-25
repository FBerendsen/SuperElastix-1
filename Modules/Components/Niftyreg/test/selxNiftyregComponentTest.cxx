/*=========================================================================
 *
 *  Copyright Leiden University Medical Center, Erasmus University Medical
 *  Center and contributors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "selxSuperElastixFilterCustomComponents.h"

#include "selxRegistrationController.h"

#include "selxNiftyregReadImageComponent.h"
#include "selxNiftyregWriteImageComponent.h"
#include "selxNiftyregWriteImageComponent.h"
#include "selxItkToNiftiImageSourceReferenceComponent.h"
#include "selxNiftiToItkImageSinkComponent.h"
#include "selxItkImageSource.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "selxNiftyregf3dComponent.h"
#include "selxDataManager.h"
#include "gtest/gtest.h"

namespace selx
{
class NiftyregComponentTest : public ::testing::Test
{
public:

  typedef std::unique_ptr< Blueprint >                       BlueprintPointer;
  typedef itk::UniquePointerDataObjectDecorator< Blueprint > BlueprintITKType;
  typedef BlueprintITKType::Pointer                          BlueprintITKPointer;
  typedef Blueprint::ParameterMapType                        ParameterMapType;
  typedef Blueprint::ParameterValueType                      ParameterValueType;
  typedef DataManager                                        DataManagerType;

  /** register all example components */
  typedef TypeList < Niftyregf3dComponent< float >,
    NiftyregReadImageComponent< float >,
    NiftyregWriteImageComponent< float >,
    ItkToNiftiImageSourceReferenceComponent< 2, float >,
    NiftiToItkImageSinkComponent<2, float>,
    ItkImageSourceComponent<2, float>,
    ItkToNiftiImageSourceReferenceComponent< 3, float >,
    NiftiToItkImageSinkComponent<3, float>,
    ItkImageSourceComponent<3, float>,
    RegistrationControllerComponent< >> RegisterComponents;

  typedef SuperElastixFilterCustomComponents< RegisterComponents > SuperElastixFilterType;
  virtual void SetUp()
  {
    // Instantiate SuperElastixFilter before each test and
    // register the components we want to have available in SuperElastix
    superElastixFilter = SuperElastixFilterType::New();
    dataManager        = DataManagerType::New();
  }


  virtual void TearDown()
  {
    // Unregister all components after each test
    itk::ObjectFactoryBase::UnRegisterAllFactories();
    // Delete the SuperElastixFilter after each test
    superElastixFilter = nullptr;
  }


  // Blueprint holds a configuration for SuperElastix
  BlueprintPointer            blueprint;
  SuperElastixFilterBase::Pointer superElastixFilter;
  // Data manager provides the paths to the input and output data for unit tests
  DataManagerType::Pointer dataManager;
};

TEST_F( NiftyregComponentTest, Register2d_nifti )
{
  /** make example blueprint configuration */
  BlueprintPointer blueprint = BlueprintPointer( new Blueprint() );
  blueprint->SetComponent( "FixedImage", { { "NameOfClass", { "NiftyregReadImageComponent" } }, { "FileName", { this->dataManager->GetInputFile( "r16slice.nii.gz" ) } } } );
  blueprint->SetComponent( "MovingImage", { { "NameOfClass", { "NiftyregReadImageComponent" } }, { "FileName", { this->dataManager->GetInputFile( "r64slice.nii.gz" ) } } } );
  blueprint->SetComponent( "RegistrationMethod", { { "NameOfClass", { "Niftyregf3dComponent" } } } );
  blueprint->SetComponent( "ResultImage", { { "NameOfClass", { "NiftyregWriteImageComponent" } }, { "FileName", { this->dataManager->GetOutputFile("Nifty_warped_r64to16.nii.gz") } } });
  blueprint->SetComponent( "Controller", { { "NameOfClass", { "RegistrationControllerComponent" } } } );


  blueprint->SetConnection("FixedImage", "RegistrationMethod", { { "NameOfInterface", { "NiftyregReferenceImageInterface" } } });
  blueprint->SetConnection( "MovingImage", "RegistrationMethod", { { "NameOfInterface", { "NiftyregFloatingImageInterface" } } });
  blueprint->SetConnection("RegistrationMethod", "ResultImage", { {} }); //{ { "NameOfInterface", { "NiftyregWarpedImageInterface" } } });
  blueprint->SetConnection( "RegistrationMethod", "Controller", { {} } );
  blueprint->SetConnection("ResultImage", "Controller", { {} });

  BlueprintITKPointer superElastixFilterBlueprint = BlueprintITKType::New();
  superElastixFilterBlueprint->Set( blueprint );
  EXPECT_NO_THROW( superElastixFilter->SetBlueprint( superElastixFilterBlueprint ) );

  EXPECT_NO_THROW( superElastixFilter->Update() );
}
TEST_F(NiftyregComponentTest, ItkToNiftiImage)
{
  /** make example blueprint configuration */
  BlueprintPointer blueprint = BlueprintPointer(new Blueprint());
  blueprint->SetComponent("FixedImage", { { "NameOfClass", { "ItkToNiftiImageSourceReferenceComponent" } }, { "Dimensionality", { "3" } }, { "PixelType", { "float" } } } );
  blueprint->SetComponent("ResultImage", { { "NameOfClass", { "NiftyregWriteImageComponent" } }, { "FileName", { this->dataManager->GetOutputFile("ItkToNiftiImage_converted.nii.gz") } } });
  blueprint->SetComponent("Controller", { { "NameOfClass", { "RegistrationControllerComponent" } } });

  blueprint->SetConnection("FixedImage", "ResultImage", { { "NameOfInterface", { "NiftyregWarpedImageInterface" } } });
  blueprint->SetConnection("ResultImage", "Controller", { { "NameOfInterface", { "AfterRegistrationInterface" } } });
  

  BlueprintITKPointer superElastixFilterBlueprint = BlueprintITKType::New();
  superElastixFilterBlueprint->Set(blueprint);
  EXPECT_NO_THROW(superElastixFilter->SetBlueprint(superElastixFilterBlueprint));

  // Set up the readers and writers
  auto fixedImageReader = itk::ImageFileReader<itk::Image<float, 3>>::New();
  fixedImageReader->SetFileName(dataManager->GetInputFile("sphereA3d.mhd"));

  // Connect SuperElastix in an itk pipeline
  superElastixFilter->SetInput("FixedImage", fixedImageReader->GetOutput());

  //EXPECT_NO_THROW(superElastixFilter->Update());
  superElastixFilter->Update();
}

TEST_F(NiftyregComponentTest, NiftiToItkImage)
{
  /** make example blueprint configuration */
  BlueprintPointer blueprint = BlueprintPointer(new Blueprint());
  // TODO proper 3d nii.gz input data
  //blueprint->SetComponent("FixedImage", { { "NameOfClass", { "NiftyregReadImageComponent" } }, { "FileName", { this->dataManager->GetInputFile("r16slice.nii.gz") } } });
  blueprint->SetComponent("FixedImage", { { "NameOfClass", { "NiftyregReadImageComponent" } }, { "FileName", { this->dataManager->GetOutputFile("ItkToNiftiImage_converted.nii.gz") } } });
  blueprint->SetComponent("ResultDomainImage", { { "NameOfClass", { "ItkImageSourceComponent" } } });
  blueprint->SetComponent("ResultImage", { { "NameOfClass", { "NiftiToItkImageSinkComponent" } }, { "Dimensionality", { "3" } }, { "PixelType", { "float" } } });
  blueprint->SetComponent("Controller", { { "NameOfClass", { "RegistrationControllerComponent" } } });

  blueprint->SetConnection("FixedImage", "ResultImage", { { "NameOfInterface", { "NiftyregWarpedImageInterface" } } });
  blueprint->SetConnection("ResultDomainImage", "ResultImage", { { "NameOfInterface", { "itkImageDomainFixedInterface" } } });
  blueprint->SetConnection("ResultImage", "Controller", { { "NameOfInterface", { "AfterRegistrationInterface" } } });


  BlueprintITKPointer superElastixFilterBlueprint = BlueprintITKType::New();
  superElastixFilterBlueprint->Set(blueprint);
  EXPECT_NO_THROW(superElastixFilter->SetBlueprint(superElastixFilterBlueprint));

  // Set up the readers and writers
  auto fixedDomainImageReader = itk::ImageFileReader<itk::Image<float, 3>>::New();
  fixedDomainImageReader->SetFileName(this->dataManager->GetOutputFile("ItkToNiftiImage_converted.nii.gz"));

  superElastixFilter->SetInput("ResultDomainImage", fixedDomainImageReader->GetOutput());

  auto fixedImageWriter = itk::ImageFileWriter<itk::Image<float, 3>>::New();
  fixedImageWriter->SetFileName(dataManager->GetOutputFile("NiftiToItkImage_converted.mhd"));

  // Connect SuperElastix in an itk pipeline
  fixedImageWriter->SetInput(superElastixFilter->GetOutput<itk::Image<float, 3>>("ResultImage") );

  //EXPECT_NO_THROW(superElastixFilter->Update());
  fixedImageWriter->Update();
  
}

TEST_F(NiftyregComponentTest, Register2d_itkImages)
{
  /** make example blueprint configuration */
  // Set max iterations to 1 for speedy tests.
  BlueprintPointer blueprint = BlueprintPointer(new Blueprint());


  blueprint->SetComponent("RegistrationMethod", { { "NameOfClass", { "Niftyregf3dComponent" } }, 
                                                  { "Metric", { "LNCC" } },
                                                  { "NumberOfResolutions", { "1" } }, 
                                                  { "NumberOfIterations", { "1" } }, 
                                                  { "Optimizer", { "Gradient" } },
                                                  { "GridSpacingInVoxels", { "8", "8" } } });
  
  blueprint->SetComponent("FixedImage", { { "NameOfClass", { "ItkToNiftiImageSourceReferenceComponent" } }, { "Dimensionality", { "2" } }, { "PixelType", { "float" } } });
  blueprint->SetComponent("MovingImage", { { "NameOfClass", { "ItkToNiftiImageSourceReferenceComponent" } }, { "Dimensionality", { "2" } }, { "PixelType", { "float" } } });
  blueprint->SetComponent("ResultImage", { { "NameOfClass", { "NiftiToItkImageSinkComponent" } }, { "Dimensionality", { "2" } }, { "PixelType", { "float" } } });
  blueprint->SetComponent("Controller", { { "NameOfClass", { "RegistrationControllerComponent" } } });

  blueprint->SetConnection("FixedImage", "RegistrationMethod", { { "NameOfInterface", { "NiftyregReferenceImageInterface" } } });
  blueprint->SetConnection("MovingImage", "RegistrationMethod", { { "NameOfInterface", { "NiftyregFloatingImageInterface" } } });
  blueprint->SetConnection("FixedImage", "ResultImage", { { "NameOfInterface", { "itkImageDomainFixedInterface" } } });
  blueprint->SetConnection("RegistrationMethod", "ResultImage", { { "NameOfInterface", { "NiftyregWarpedImageInterface" } } });
  blueprint->SetConnection("RegistrationMethod", "Controller", { {} });
  blueprint->SetConnection("ResultImage", "Controller", { {} });

  blueprint->Write(dataManager->GetOutputFile("Niftyreg_WBIR.dot"));

  // Set up the readers and writers
  typedef itk::Image< float, 2 >              Image2DType;
  typedef itk::ImageFileReader< Image2DType > ImageReader2DType;
  typedef itk::ImageFileWriter< Image2DType > ImageWriter2DType;

  ImageReader2DType::Pointer fixedImageReader = ImageReader2DType::New();
  fixedImageReader->SetFileName(dataManager->GetInputFile("coneA2d64.mhd"));

  ImageReader2DType::Pointer movingImageReader = ImageReader2DType::New();
  movingImageReader->SetFileName(dataManager->GetInputFile("coneB2d64.mhd"));

  ImageWriter2DType::Pointer resultImageWriter = ImageWriter2DType::New();
  resultImageWriter->SetFileName(dataManager->GetOutputFile("Niftyreg_WBIR_Image.mhd"));

  //DisplacementImageWriter2DType::Pointer resultDisplacementWriter = DisplacementImageWriter2DType::New();
  //resultDisplacementWriter->SetFileName(dataManager->GetOutputFile("Niftyreg_WBIR_Displacement.mhd"));

  // Connect SuperElastix in an itk pipeline
  superElastixFilter->SetInput("FixedImage", fixedImageReader->GetOutput());
  superElastixFilter->SetInput("MovingImage", movingImageReader->GetOutput());
  resultImageWriter->SetInput(superElastixFilter->GetOutput< Image2DType >("ResultImage"));
  //resultDisplacementWriter->SetInput(superElastixFilter->GetOutput< DisplacementImage2DType >("ResultDisplacementFieldSink"));

  BlueprintITKPointer superElastixFilterBlueprint = BlueprintITKType::New();
  superElastixFilterBlueprint->Set(blueprint);
  EXPECT_NO_THROW(superElastixFilter->SetBlueprint(superElastixFilterBlueprint));

  //Optional Update call
  //superElastixFilter->Update();

  // Update call on the writers triggers SuperElastix to configure and execute
  EXPECT_NO_THROW(resultImageWriter->Update());
}

TEST_F(NiftyregComponentTest, WBIRDemo)
{
  /** make example blueprint configuration */
  BlueprintPointer blueprint = BlueprintPointer(new Blueprint());

  
  blueprint->SetComponent("RegistrationMethod", { { "NameOfClass", { "Niftyregf3dComponent" } } });
  blueprint->SetComponent("FixedImage", { { "NameOfClass", { "ItkToNiftiImageSourceReferenceComponent" } }, { "Dimensionality", { "2" } }, { "PixelType", { "float" } } });
  blueprint->SetComponent("MovingImage", { { "NameOfClass", { "ItkToNiftiImageSourceReferenceComponent" } }, { "Dimensionality", { "2" } }, { "PixelType", { "float" } } });
  blueprint->SetComponent("ResultImage", { { "NameOfClass", { "NiftiToItkImageSinkComponent" } }, { "Dimensionality", { "2" } }, { "PixelType", { "float" } } });
  blueprint->SetComponent("Controller", { { "NameOfClass", { "RegistrationControllerComponent" } } });

  blueprint->SetConnection("FixedImage", "RegistrationMethod", { { "NameOfInterface", { "NiftyregReferenceImageInterface" } } });
  blueprint->SetConnection("MovingImage", "RegistrationMethod", { { "NameOfInterface", { "NiftyregFloatingImageInterface" } } });
  blueprint->SetConnection("FixedImage", "ResultImage", { { "NameOfInterface", { "itkImageDomainFixedInterface" } } });
  blueprint->SetConnection("RegistrationMethod", "ResultImage", { { "NameOfInterface", { "NiftyregWarpedImageInterface" } } });
  blueprint->SetConnection("RegistrationMethod", "Controller", { {} });
  blueprint->SetConnection("ResultImage", "Controller", { {} });

  blueprint->Write(dataManager->GetOutputFile("Niftyreg_WBIR.dot"));

  // Set up the readers and writers
  typedef itk::Image< float, 2 >              Image2DType;
  typedef itk::ImageFileReader< Image2DType > ImageReader2DType;
  typedef itk::ImageFileWriter< Image2DType > ImageWriter2DType;

  ImageReader2DType::Pointer fixedImageReader = ImageReader2DType::New();
  fixedImageReader->SetFileName(dataManager->GetInputFile("coneA2d64.mhd"));

  ImageReader2DType::Pointer movingImageReader = ImageReader2DType::New();
  movingImageReader->SetFileName(dataManager->GetInputFile("coneB2d64.mhd"));

  ImageWriter2DType::Pointer resultImageWriter = ImageWriter2DType::New();
  resultImageWriter->SetFileName(dataManager->GetOutputFile("Niftyreg_WBIR_Image.mhd"));

  //DisplacementImageWriter2DType::Pointer resultDisplacementWriter = DisplacementImageWriter2DType::New();
  //resultDisplacementWriter->SetFileName(dataManager->GetOutputFile("Niftyreg_WBIR_Displacement.mhd"));

  // Connect SuperElastix in an itk pipeline
  superElastixFilter->SetInput("FixedImage", fixedImageReader->GetOutput());
  superElastixFilter->SetInput("MovingImage", movingImageReader->GetOutput());
  resultImageWriter->SetInput(superElastixFilter->GetOutput< Image2DType >("ResultImage"));
  //resultDisplacementWriter->SetInput(superElastixFilter->GetOutput< DisplacementImage2DType >("ResultDisplacementFieldSink"));

  BlueprintITKPointer superElastixFilterBlueprint = BlueprintITKType::New();
  superElastixFilterBlueprint->Set(blueprint);
  EXPECT_NO_THROW(superElastixFilter->SetBlueprint(superElastixFilterBlueprint));

  //Optional Update call
  //superElastixFilter->Update();

  // Update call on the writers triggers SuperElastix to configure and execute
  EXPECT_NO_THROW(resultImageWriter->Update());
  //EXPECT_NO_THROW(resultDisplacementWriter->Update());
}
}
