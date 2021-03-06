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

#include "selxTypeList.h"

//Component group SinksAndSources
#include "selxDisplacementFieldItkImageFilterSinkComponent.h"
#include "selxItkImageSourceComponent.h"
#include "selxItkImageSinkComponent.h"

namespace selx
{
using ModuleSinksAndSourcesComponents = selx::TypeList<
  DisplacementFieldItkImageFilterSinkComponent< 2, float >,
  DisplacementFieldItkImageFilterSinkComponent< 3, float >,
  ItkImageSinkComponent< 2, float >,
  ItkImageSinkComponent< 3, short >,
  ItkImageSinkComponent< 3, float >,
  ItkImageSourceComponent< 2, float >,
  ItkImageSourceComponent< 2, short >,
  ItkImageSourceComponent< 2, unsigned char >, // for masks
  ItkImageSourceComponent< 3, float >,
  ItkImageSourceComponent< 3, short >,
  ItkImageSourceComponent< 3, unsigned char > // for masks
  >;
}
