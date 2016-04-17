#include <iostream>
#include <string>

// Generally always include these
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>

// For reading in images
#include <vtkSmartPointer.h>
#include <vtkJPEGReader.h>

#include <vtkImageDataToPointSet.h>
#include <vtkImageDataGeometryFilter.h>

#include <vtkPolyData.h>
#include <vtkDataSetMapper.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <vtkUnstructuredGrid.h>
#include <vtkCleanPolyData.h>
#include <vtkAppendPolyData.h>
#include <vtkStructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkImageData.h>

#include <vtkAppendFilter.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkClipDataSet.h>
#include <vtkPolyDataMapper.h>
#include <vtkPoints.h>
#include <vtkStructuredGrid.h>
#include <vtkProbeFilter.h>
#include <vtkDelaunay3D.h>

#include <vtkPlaneSource.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkClipDataSet.h>
#include <vtkPlane.h>
#include <vtkSampleFunction.h>
#include <vtkContourFilter.h>
#include <vtkOutlineFilter.h>
#include <vtkPlaneWidget.h>

#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>

// Variables holding plane position and orientation.
double center[3];
double* bounds;
double width;
double height;
double length;

// Define image plane widget
vtkSmartPointer<vtkPlaneWidget> sagittalClipPlaneWidget = vtkSmartPointer<vtkPlaneWidget>::New();
vtkSmartPointer<vtkPlaneWidget> axialClipPlaneWidget = vtkSmartPointer<vtkPlaneWidget>::New();
vtkSmartPointer<vtkPlaneWidget> coronalClipPlaneWidget = vtkSmartPointer<vtkPlaneWidget>::New();

vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();

// Define clipping filter
vtkSmartPointer<vtkPlane> sagittalClipPlane = vtkSmartPointer<vtkPlane>::New();
vtkSmartPointer<vtkClipDataSet> sagittalClipDataSet = vtkSmartPointer<vtkClipDataSet>::New();

vtkSmartPointer<vtkPlane> axialClipPlane = vtkSmartPointer<vtkPlane>::New();
vtkSmartPointer<vtkClipDataSet> axialClipDataSet = vtkSmartPointer<vtkClipDataSet>::New();

vtkSmartPointer<vtkPlane> coronalClipPlane = vtkSmartPointer<vtkPlane>::New();
vtkSmartPointer<vtkClipDataSet> coronalClipDataSet = vtkSmartPointer<vtkClipDataSet>::New();


// Define interaction style
class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static KeyPressInteractorStyle* New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress()
    {
        // Get the keypress
        vtkRenderWindowInteractor *rwi = this->Interactor;
        std::string key = rwi->GetKeySym();

        // Output the key that was pressed
        std::cout << "Pressed " << key << std::endl;

        // Translate (with normal in +Z direction).
        if (key == "Up" || key == "Down") {
            double* sagittalClipPlaneCenter = sagittalClipPlaneWidget->GetCenter();
            double* sagittalClipPlaneNormal = sagittalClipPlaneWidget->GetNormal();

            if (key == "Up")
                sagittalClipPlaneCenter[2] += 1.0; // Translate plane in the direction of its normal (+Z)
            else
                sagittalClipPlaneCenter[2] -= 1.0; // Translate plane in the direction of its normal (-Z)

            // Check to make sure clip plane is within bounds.
            double zAxisLowerBound = bounds[4];
            double zAxisUpperBound = bounds[5];
            if (sagittalClipPlaneCenter[2] < zAxisLowerBound)
                sagittalClipPlaneCenter[2] = zAxisLowerBound;
            if (sagittalClipPlaneCenter[2] > zAxisUpperBound)
                sagittalClipPlaneCenter[2] = zAxisUpperBound;

            sagittalClipPlaneWidget->SetCenter(sagittalClipPlaneCenter);
            sagittalClipPlaneWidget->PlaceWidget(sagittalClipPlaneCenter[0], sagittalClipPlaneCenter[0],
                sagittalClipPlaneCenter[1] - width, sagittalClipPlaneCenter[1] + width,
                sagittalClipPlaneCenter[2] - length, sagittalClipPlaneCenter[2] + length);
            sagittalClipPlaneWidget->SetNormal(0, 0, 1);

            renderWindow->Render();
        }

        if (key == "Left" || key == "Right") {
            double* axialClipPlaneCenter = axialClipPlaneWidget->GetCenter();
            double* axialClipPlaneNormal = axialClipPlaneWidget->GetNormal();

            if (key == "Right")
                axialClipPlaneCenter[1] += 1.0; // Translate plane in the direction of its normal (+Y)
            else
                axialClipPlaneCenter[1] -= 1.0; // Translate plane in the direction of its normal (-Y)

            // Check to make sure clip plane is within bounds.
            double yAxisLowerBound = bounds[2];
            double yAxisUpperBound = bounds[3];
            if (axialClipPlaneCenter[1] < yAxisLowerBound)
                axialClipPlaneCenter[1] = yAxisLowerBound;
            if (axialClipPlaneCenter[1] > yAxisUpperBound)
                axialClipPlaneCenter[1] = yAxisUpperBound;

            axialClipPlaneWidget->SetCenter(axialClipPlaneCenter);
            axialClipPlaneWidget->PlaceWidget(axialClipPlaneCenter[0], axialClipPlaneCenter[0],
                axialClipPlaneCenter[1] - width, axialClipPlaneCenter[1] + width,
                axialClipPlaneCenter[2] - height, axialClipPlaneCenter[2] + height);
            axialClipPlaneWidget->SetNormal(0, 1, 0);

            renderWindow->Render();
        }

        if (key == "a" || key == "z") {
            double* coronalClipPlaneCenter = coronalClipPlaneWidget->GetCenter();
            double* coronalClipPlaneNormal = coronalClipPlaneWidget->GetNormal();

            if (key == "a")
                coronalClipPlaneCenter[0] += 1.0; // Translate plane in the direction of its normal (+X)
            else
                coronalClipPlaneCenter[0] -= 1.0; // Translate plane in the direction of its normal (-X)

                                                // Check to make sure clip plane is within bounds.
            double xAxisLowerBound = bounds[0];
            double xAxisUpperBound = bounds[1];
            if (coronalClipPlaneCenter[0] < xAxisLowerBound)
                coronalClipPlaneCenter[0] = xAxisLowerBound;
            if (coronalClipPlaneCenter[0] > xAxisUpperBound)
                coronalClipPlaneCenter[0] = xAxisUpperBound;

            coronalClipPlaneWidget->SetCenter(coronalClipPlaneCenter);
            coronalClipPlaneWidget->PlaceWidget(coronalClipPlaneCenter[0], coronalClipPlaneCenter[0],
                coronalClipPlaneCenter[1] - length, coronalClipPlaneCenter[1] + length,
                coronalClipPlaneCenter[2] - height, coronalClipPlaneCenter[2] + height);
            coronalClipPlaneWidget->SetNormal(1, 0, 0);

            renderWindow->Render();
        }

        if (key == "space") {
            sagittalClipPlaneWidget->GetPlane(sagittalClipPlane);
            sagittalClipDataSet->SetClipFunction(sagittalClipPlane);
            sagittalClipDataSet->Update();

            axialClipPlaneWidget->GetPlane(axialClipPlane);
            axialClipDataSet->SetClipFunction(axialClipPlane);
            axialClipDataSet->Update();

            coronalClipPlaneWidget->GetPlane(coronalClipPlane);
            coronalClipDataSet->SetClipFunction(coronalClipPlane);
            coronalClipDataSet->Update();

            renderWindow->Render();
        }

        // Forward events
        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

};
vtkStandardNewMacro(KeyPressInteractorStyle);

int main(int argc, char* argv[])
{
    vtkSmartPointer<vtkJPEGReader> reader;
    vtkSmartPointer<vtkImageDataGeometryFilter> imageDataGeometryFilter;

    vtkSmartPointer<vtkTransform> rotationTransform;
    vtkSmartPointer<vtkTransform> translateTransform;
    vtkSmartPointer<vtkTransformPolyDataFilter> rotationTransformFilter;
    vtkSmartPointer<vtkTransformPolyDataFilter> translateTransformFilter;

    vtkSmartPointer<vtkPolyData> myImageData;

    vtkSmartPointer<vtkAppendPolyData> appendPolyDataFilter =
        vtkSmartPointer<vtkAppendPolyData>::New();

    /*
    Read in each image, convert it to a vtkStructuredGrid, then rotate it by
    angleOffset degrees and append it to the rest of the read images.
    */
    std::vector<std::string> imageFileNames;
    for (std::string imageFileName; std::getline(std::cin, imageFileName);) {
        imageFileNames.push_back(imageFileName);
    }

    std::cout << imageFileNames.size() << " files total." << std::endl;
    std::cout << "Image file names:" << std::endl;

    /*
    Compute amount (in degrees) to rotate each image plane by, so that the
    image planes are centered around initialAngle degrees.
    */
    // Vector holding amount (in degrees) to rotate each image plane by.
    std::vector<double> imageRotationAngles;

    int N = imageFileNames.size(); // Number of image planes.
    double centerAngle = 0.0; // Center image planes around this angle.
    double angleOffset = 4.0; // Angle offset between image planes.
    double translateOffset = 20.0; // How far away the plane is from the center of the transducer.

    for (std::vector<std::string>::size_type i = 0; i < imageFileNames.size(); i++) {
        imageRotationAngles.push_back(centerAngle + (int(i) - N / 2)*angleOffset);
        // If there are an even number of image planes, we shift all rotations
        // forward by half of angleOffset so that we are still centered on
        // centerAngle.
        if (imageFileNames.size() % 2 == 0)
            imageRotationAngles[i] += angleOffset / 2.0;
    }

    // Apply computed rotation angles to each image plane.
    for (std::vector<std::string>::size_type i = 0; i < imageFileNames.size(); i++) {
        std::string imageFileName = imageFileNames[i];
        std::cout << imageFileName << std::endl;

        // Read in image data as vtkImageData.
        reader = vtkSmartPointer<vtkJPEGReader>::New();
        reader->SetFileName(imageFileName.c_str());
        

        // Code for converting from 
        // vtkImageData -> vtkPolyData -> vtkUnstructuredGrid.
        imageDataGeometryFilter = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
        imageDataGeometryFilter->SetInputConnection(reader->GetOutputPort());
        imageDataGeometryFilter->Update();

        // Offset the image slightly by the transducer's radius.
        translateTransform = vtkSmartPointer<vtkTransform>::New();
        translateTransform->Translate(translateOffset, 0.0, 0.0);
        std::cout << imageRotationAngles[i] << std::endl;


        translateTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        translateTransformFilter->SetTransform(translateTransform);
        translateTransformFilter->SetInputConnection(imageDataGeometryFilter->GetOutputPort());
        translateTransformFilter->Update();

        // Rotate the image.
        rotationTransform = vtkSmartPointer<vtkTransform>::New();
        rotationTransform->RotateWXYZ(imageRotationAngles[i], 0, 1, 0);

        rotationTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        rotationTransformFilter->SetTransform(rotationTransform);
        rotationTransformFilter->SetInputConnection(translateTransformFilter->GetOutputPort());
        rotationTransformFilter->Update();

        myImageData = vtkSmartPointer<vtkPolyData>::New();
        myImageData->ShallowCopy(rotationTransformFilter->GetOutput());

        // Consider skipping this step and just adding data to the appendFilter
        // to more directly convert to vtkUnstructuredGrid data.
#if VTK_MAJOR_VERSION <= 5
        appendPolyDataFilter->AddInputConnection(myImageData->GetProducerPort());
#else
        appendPolyDataFilter->AddInputData(myImageData);
#endif
        appendPolyDataFilter->Update();
    }

    // Remove any duplicate points.
    vtkSmartPointer<vtkCleanPolyData> cleanFilter =
        vtkSmartPointer<vtkCleanPolyData>::New();
    cleanFilter->SetInputConnection(appendPolyDataFilter->GetOutputPort());
    cleanFilter->Update();

    // Convert vtkPolyData to vtkUnstructuredGrid using vtkAppendFilter.
    vtkSmartPointer<vtkAppendFilter> appendFilter =
        vtkSmartPointer<vtkAppendFilter>::New();
#if VTK_MAJOR_VERSION <= 5
    appendFilter->AddInput(sphereSource->GetOutput());
#else
    appendFilter->AddInputData(cleanFilter->GetOutput());
#endif
    appendFilter->Update();

    
    // Triangulate the image data.
    vtkSmartPointer<vtkDelaunay3D> delaunayFilter =
    vtkSmartPointer<vtkDelaunay3D>::New();
    #if VTK_MAJOR_VERSION <= 5
    delaunayFilter->SetInput(triangleFilter);
    #else
    delaunayFilter->SetInputConnection(appendFilter->GetOutputPort());
    #endif
    delaunayFilter->Update();
    

    /*
    Sample vtkUnstructuredGrid into a uniformly sampled vtkStructuredGrid.

    1. Get x, y, z extents of the vtkUnstructuredGrid.
    2. Determine what spacing to use in these dimensions. Based on this
    spacing, create a grid of points for the vtkStructuredGrid.
    3. Determine the value at each of these points using vtkProbeFilter
    for interpolation.
    */

    // Step 1: Get x, y, z extents of the vtkUnstructuredGrid.

    // Cast output of appendFilter to vtkUnstructuredGrid

    bounds = cleanFilter->GetOutput()->GetBounds();
    center[0] = (bounds[0] + bounds[1]) / 2.0;
    center[1] = (bounds[2] + bounds[3]) / 2.0;
    center[2] = (bounds[4] + bounds[5]) / 2.0;

    std::cout << std::endl << "Bounds: " << std::endl;
    std::cout << "x: (" << bounds[0] << ", " << bounds[1] << ")" << std::endl;
    std::cout << "y: (" << bounds[2] << ", " << bounds[3] << ")" << std::endl;
    std::cout << "z: (" << bounds[4] << ", " << bounds[5] << ")" << std::endl;
    // Step 2: Use bounds to determine locations where we can sample the 
    // vtkStructuredGrid.

    // Determine spacing in each dimension based on bounds and grid size.
    int numXPoints = 10;
    int numYPoints = 10;
    int numZPoints = 10;

    double spacingX = (bounds[1] - bounds[0]) / (double)(numXPoints);
    double spacingY = (bounds[3] - bounds[2]) / (double)(numYPoints);
    double spacingZ = (bounds[5] - bounds[4]) / (double)(numZPoints);

    std::cout << std::endl << "Spacings: " << std::endl;
    std::cout << "x: " << spacingX << std::endl;
    std::cout << "y: " << spacingY << std::endl;
    std::cout << "z: " << spacingZ << std::endl;

    // Construct vtkStructuredGrid based on this example:
    // http://www.vtk.org/Wiki/VTK/Examples/Cxx/StructuredGrid/StructuredGrid
    vtkSmartPointer<vtkStructuredGrid> structuredGrid =
    vtkSmartPointer<vtkStructuredGrid>::New();

    vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

    for (double k = bounds[4]; k < bounds[5]; k += spacingZ) {
        for (double j = bounds[2]; j < bounds[3]; j += spacingY) {
            for (double i = bounds[0]; i < bounds[1]; i += spacingX) {
            points->InsertNextPoint(i, j, k);
            }
        }
    }

    structuredGrid->SetDimensions(numXPoints, numYPoints, numZPoints);
    structuredGrid->SetPoints(points);


    // 3. Interpolate on the given vtkUnstructuredGrid images to compute
    // the correct value to assign each of the vtkStructuredGrid points.
    vtkSmartPointer<vtkProbeFilter> probeFilter =
    vtkSmartPointer<vtkProbeFilter>::New();
    probeFilter->SetSourceConnection(delaunayFilter->GetOutputPort());
#if VTK_MAJOR_VERSION <= 5
    probeFilter->SetInput(structuredGrid); // Interpolate 'Source' at these points
#else
    probeFilter->SetInputData(structuredGrid); // Interpolate 'Source' at these points
#endif
    probeFilter->Update();
    std::cout << probeFilter->GetOutput() << std::endl;
    

    // Triangulate structured grid before clipping.
    vtkSmartPointer<vtkDataSetTriangleFilter> triangleFilter =
    vtkSmartPointer<vtkDataSetTriangleFilter>::New();
    triangleFilter->SetInputConnection(probeFilter->GetOutputPort());
    triangleFilter->Update();
    

    // Apply vtkClipDataSet filter for interpolation.    
    // Create a vtkPlane (implicit function) to interpolate over.
    sagittalClipPlane->SetOrigin(0.0, 0.0, 0.0);
    sagittalClipPlane->SetNormal(0.0, 0.0, 1.0);

    axialClipPlane->SetOrigin(0.0, 0.0, 0.0);
    axialClipPlane->SetNormal(0.0, 1.0, 0.0);
    
    coronalClipPlane->SetOrigin(0.0, 0.0, 0.0);
    coronalClipPlane->SetNormal(1.0, 0.0, 0.0);

    vtkSmartPointer<vtkOutlineFilter> imageVolumeOutline =
        vtkSmartPointer<vtkOutlineFilter>::New();
    imageVolumeOutline->SetInputConnection(appendFilter->GetOutputPort());

    
    // I have no idea how the PlaceWidget() function works. I placed the
    // planes using the guess-and-check method.
    height = bounds[5] - bounds[4];
    width = bounds[3] - bounds[2];
    length = bounds[1] - bounds[0];

    sagittalClipPlaneWidget->SetInputData(imageVolumeOutline->GetOutput());
    sagittalClipPlaneWidget->SetHandleSize(0.0001);
    sagittalClipPlaneWidget->GetPlaneProperty()->SetColor(0.0, 0.0, 1.0);
    sagittalClipPlaneWidget->SetCenter(center);
    sagittalClipPlaneWidget->PlaceWidget(center[0], center[0], 
        center[1] - width, center[1] + width, 
        center[2] - length, center[2] + length);
    sagittalClipPlaneWidget->SetNormal(0, 0, 1);

    axialClipPlaneWidget->SetInputData(imageVolumeOutline->GetOutput());
    axialClipPlaneWidget->SetHandleSize(0.0001);
    axialClipPlaneWidget->GetPlaneProperty()->SetColor(0.0, 1.0, 0.0);
    axialClipPlaneWidget->SetCenter(center);
    axialClipPlaneWidget->PlaceWidget(center[0], center[0], 
        center[1] - width, center[1] + width, 
        center[2] - height, center[2] + height);
    axialClipPlaneWidget->SetNormal(0, 1, 0);

    coronalClipPlaneWidget->SetInputData(imageVolumeOutline->GetOutput());
    coronalClipPlaneWidget->SetHandleSize(0.0001);
    coronalClipPlaneWidget->GetPlaneProperty()->SetColor(1.0, 0.0, 0.0);
    coronalClipPlaneWidget->SetCenter(center);
    coronalClipPlaneWidget->PlaceWidget(center[0], center[0], 
        center[1] - length, center[1] + length,
        center[2] - height, center[2] + height);
    coronalClipPlaneWidget->SetNormal(1, 0, 0);

    
    // Perform the clipping.
    sagittalClipDataSet->SetClipFunction(sagittalClipPlane);
    axialClipDataSet->SetClipFunction(axialClipPlane);
    coronalClipDataSet->SetClipFunction(coronalClipPlane);

    
 #if VTK_MAJOR_VERSION <= 5
    sagittalClipDataSet->SetInput(triangleFilter->GetOutput());
    axialClipDataSet->SetInput(triangleFilter->GetOutput());
    coronalClipDataSet->SetInput(triangleFilter->GetOutput());
#else
    sagittalClipDataSet->SetInputData(triangleFilter->GetOutput());
    axialClipDataSet->SetInputData(triangleFilter->GetOutput());
    coronalClipDataSet->SetInputData(triangleFilter->GetOutput());
#endif
    sagittalClipDataSet->Update();
    axialClipDataSet->Update();
    coronalClipDataSet->Update();
    

    // Create mapper and actor for clipped image volume.
    vtkSmartPointer<vtkDataSetMapper> sagittalClippedVolumeMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
#if VTK_MAJOR_VERSION <= 5
    sagittalClippedVolumeMapper->SetInputConnection(unstructuredGrid->GetProducerPort());
#else
    sagittalClippedVolumeMapper->SetInputData(sagittalClipDataSet->GetOutput());
#endif
    vtkSmartPointer<vtkActor> sagittalClippedVolumeActor =
        vtkSmartPointer<vtkActor>::New();
    sagittalClippedVolumeActor->SetMapper(sagittalClippedVolumeMapper);


    vtkSmartPointer<vtkDataSetMapper> axialClippedVolumeMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
#if VTK_MAJOR_VERSION <= 5
    axialClippedVolumeMapper->SetInputConnection(unstructuredGrid->GetProducerPort());
#else
    axialClippedVolumeMapper->SetInputData(axialClipDataSet->GetOutput());
#endif
    vtkSmartPointer<vtkActor> axialClippedVolumeActor =
        vtkSmartPointer<vtkActor>::New();
    axialClippedVolumeActor->SetMapper(axialClippedVolumeMapper);


    vtkSmartPointer<vtkDataSetMapper> coronalClippedVolumeMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
#if VTK_MAJOR_VERSION <= 5
    coronalClippedVolumeMapper->SetInputConnection(unstructuredGrid->GetProducerPort());
#else
    coronalClippedVolumeMapper->SetInputData(coronalClipDataSet->GetOutput());
#endif
    vtkSmartPointer<vtkActor> coronalClippedVolumeActor =
        vtkSmartPointer<vtkActor>::New();
    coronalClippedVolumeActor->SetMapper(coronalClippedVolumeMapper);


    // Create mapper and actor for image volume outline.
    vtkSmartPointer<vtkDataSetMapper> imageVolumeMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
    imageVolumeMapper->SetInputConnection(imageVolumeOutline->GetOutputPort());
    //gridMapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> imageVolumeActor =
        vtkSmartPointer<vtkActor>::New();
    imageVolumeActor->SetMapper(imageVolumeMapper);
    //gridActor->GetProperty()->SetColor(0.0, 0.0, 1.0); //(R,G,B)
    //gridActor->GetProperty()->SetPointSize(3);

    // Visualization.
    // Center camera on image volume.
    vtkSmartPointer<vtkCamera> imageVolumeCamera =
        vtkSmartPointer<vtkCamera>::New();
    imageVolumeCamera->SetPosition(0, -60, 0);
    imageVolumeCamera->SetFocalPoint(center);

    // Camera for .
    vtkSmartPointer<vtkCamera> sagittalSliceCamera =
        vtkSmartPointer<vtkCamera>::New();
    sagittalSliceCamera->SetFocalPoint(center);
    sagittalSliceCamera->SetPosition(0, 0, -60);

    // Center camera on image volume.
    vtkSmartPointer<vtkCamera> axialSliceCamera =
        vtkSmartPointer<vtkCamera>::New();
    axialSliceCamera->SetPosition(0, -60, 0);
    axialSliceCamera->SetFocalPoint(center);

    // Center camera on image volume.
    vtkSmartPointer<vtkCamera> coronalSliceCamera =
        vtkSmartPointer<vtkCamera>::New();
    coronalSliceCamera->SetPosition(-60, 0, 0);
    coronalSliceCamera->SetFocalPoint(center);


    // Divide the main window into four separate sections.
    // The main section on the top shows the 3D image volume.
    // The row of three lower sections hold the 2D axial, coronal, and 
    // sagittal slices of the image volume.

    // Define viewport ranges
    // (xmin, ymin, xmax, ymax)
    double volumeViewport[4] = { 0.0, 0.3, 1.0, 1.0 };
    double axialViewport[4] = { 0.0, 0.0, 1.0 / 3.0, 0.3 };
    double coronalViewport[4] = { 1.0 / 3.0, 0.0, 2.0 / 3.0, 0.3 };
    double sagittalViewport[4] = { 2.0 / 3.0, 0.0, 1.0, 0.3 };

    // Add actors for each renderer window.
    vtkSmartPointer<vtkRenderer> volumeRenderer =
        vtkSmartPointer<vtkRenderer>::New();
    volumeRenderer->SetActiveCamera(imageVolumeCamera);
    volumeRenderer->SetViewport(volumeViewport);

    volumeRenderer->AddActor(imageVolumeActor);
    volumeRenderer->SetBackground(.8, .8, .8); // Set background color.
    volumeRenderer->RemoveAllLights();

    
    vtkSmartPointer<KeyPressInteractorStyle> style =
        vtkSmartPointer<KeyPressInteractorStyle>::New();
    style->SetCurrentRenderer(volumeRenderer);
    

    vtkSmartPointer<vtkRenderer> axialSliceRenderer =
        vtkSmartPointer<vtkRenderer>::New();
    axialSliceRenderer->SetViewport(axialViewport);
    axialSliceRenderer->SetActiveCamera(axialSliceCamera);
    axialSliceRenderer->AddActor(axialClippedVolumeActor);
    axialSliceRenderer->SetBackground(.1, .2, .3); // Set background color.

    vtkSmartPointer<vtkRenderer> coronalSliceRenderer =
        vtkSmartPointer<vtkRenderer>::New();
    coronalSliceRenderer->SetViewport(coronalViewport);
    coronalSliceRenderer->SetActiveCamera(coronalSliceCamera);
    coronalSliceRenderer->AddActor(coronalClippedVolumeActor);
    coronalSliceRenderer->SetBackground(.1, .2, .3); // Set background color.


    vtkSmartPointer<vtkRenderer> sagittalSliceRenderer =
        vtkSmartPointer<vtkRenderer>::New();
    sagittalSliceRenderer->SetViewport(sagittalViewport);
    sagittalSliceRenderer->SetActiveCamera(sagittalSliceCamera);
    sagittalSliceRenderer->AddActor(sagittalClippedVolumeActor);
    sagittalSliceRenderer->SetBackground(.1, .2, .3); // Set background color.


    renderWindow->AddRenderer(axialSliceRenderer);
    renderWindow->AddRenderer(coronalSliceRenderer);
    renderWindow->AddRenderer(sagittalSliceRenderer);
    renderWindow->AddRenderer(volumeRenderer);


    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
        vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindowInteractor->SetInteractorStyle(style);

    sagittalClipPlaneWidget->SetInteractor(renderWindowInteractor);
    sagittalClipPlaneWidget->On();

    axialClipPlaneWidget->SetInteractor(renderWindowInteractor);
    axialClipPlaneWidget->On();    
    
    coronalClipPlaneWidget->SetInteractor(renderWindowInteractor);
    coronalClipPlaneWidget->On();

    // Add orientation axes
    vtkSmartPointer<vtkAxesActor> axes =
        vtkSmartPointer<vtkAxesActor>::New();

    vtkSmartPointer<vtkOrientationMarkerWidget> widget =
        vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    widget->SetOutlineColor(0.9300, 0.5700, 0.1300);
    widget->SetOrientationMarker(axes);
    widget->SetInteractor(renderWindowInteractor);
    widget->SetViewport(0.0, 0.35, 0.2, 0.55); // bottom left corner of volume viewer
    widget->SetEnabled(1);
    widget->InteractiveOff();

    renderWindow->Render();

    // Start interactive window.
    renderWindowInteractor->Initialize();
    renderWindowInteractor->Start();

    return EXIT_SUCCESS;
}
