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

// Define image plane widget
vtkSmartPointer<vtkPlaneWidget> clipPlaneWidget = vtkSmartPointer<vtkPlaneWidget>::New();
vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();

// Define clipping filter
vtkSmartPointer<vtkPlane> clipPlane = vtkSmartPointer<vtkPlane>::New();
vtkSmartPointer<vtkClipDataSet> clipDataSet = vtkSmartPointer<vtkClipDataSet>::New();


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
            double* clipPlaneCenter = clipPlaneWidget->GetCenter();
            double* clipPlaneNormal = clipPlaneWidget->GetNormal();

            if (key == "Up")
                clipPlaneCenter[2] += 1.0; // Translate plane in the direction of its normal (+Z)
            else
                clipPlaneCenter[2] -= 1.0; // Translate plane in the direction of its normal (-Z)

            // Check to make sure clip plane is within bounds.
            double zAxisLowerBound = bounds[4];
            double zAxisUpperBound = bounds[5];
            if (clipPlaneCenter[2] < zAxisLowerBound)
                clipPlaneCenter[2] = zAxisLowerBound;
            if (clipPlaneCenter[2] > zAxisUpperBound)
                clipPlaneCenter[2] = zAxisUpperBound;

            clipPlaneWidget->SetCenter(clipPlaneCenter);
            clipPlaneWidget->PlaceWidget(clipPlaneCenter[0], clipPlaneCenter[0], 
                                         clipPlaneCenter[1] - 50.00, clipPlaneCenter[1] + 50.0, 
                                         clipPlaneCenter[2] - 50.0, clipPlaneCenter[2] + 50.0);
            clipPlaneWidget->SetNormal(0, 0, 1);

            renderWindow->Render();
        }

        if (key == "space") {
            clipPlaneWidget->GetPlane(clipPlane);
            clipDataSet->SetClipFunction(clipPlane);
            clipDataSet->Update();

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
    clipPlane->SetOrigin(0.0, 0.0, 0.0);
    clipPlane->SetNormal(0.0, 0.0, 1.0);

    vtkSmartPointer<vtkOutlineFilter> imageVolumeOutline =
        vtkSmartPointer<vtkOutlineFilter>::New();
    imageVolumeOutline->SetInputConnection(appendFilter->GetOutputPort());

    
    clipPlaneWidget->SetInputData(imageVolumeOutline->GetOutput());
    clipPlaneWidget->SetHandleSize(0.0001);
    clipPlaneWidget->GetPlaneProperty()->SetColor(0.0, 0.0, 1.0);
    clipPlaneWidget->SetCenter(center);
    double pointOne[3] = { center[0] + .5, center[1], center[2] + .5 };
    double pointTwo[3] = { center[0] - .5, center[1], center[2] - .5 };
    clipPlaneWidget->PlaceWidget(center[0], center[0], center[1] - 50.00, center[1] + 50.0, center[2] - 50.0, center[2] + 50.0);
    clipPlaneWidget->SetNormal(0, 0, 1);


    // Perform the clipping.
    clipDataSet->SetClipFunction(clipPlane);

 #if VTK_MAJOR_VERSION <= 5
    probeFilter->SetInput(gridPolyData);
    // Interpolate 'Source' at these points
#else
    clipDataSet->SetInputData(triangleFilter->GetOutput());
    // Interpolate 'Source' at these points
#endif
    clipDataSet->Update();

    
    /*
    vtkPolyData* plane = clipPlane->GetOutput();
    // Create a mapper and actor
    vtkSmartPointer<vtkPolyDataMapper> planeMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
    #if VTK_MAJOR_VERSION <= 5
    planeMapper->SetInput(plane);
    #else
    planeMapper->SetInputData(plane);
    #endif
    vtkSmartPointer<vtkActor> planeActor =
    vtkSmartPointer<vtkActor>::New();
    planeActor->SetMapper(planeMapper);
    */

    // Create mapper and actor for clipped image volume.
    vtkSmartPointer<vtkDataSetMapper> clippedVolumeMapper =
        vtkSmartPointer<vtkDataSetMapper>::New();
#if VTK_MAJOR_VERSION <= 5
    clippedVolumeMapper->SetInputConnection(unstructuredGrid->GetProducerPort());
#else
    clippedVolumeMapper->SetInputData(clipDataSet->GetOutput());
#endif
    vtkSmartPointer<vtkActor> clippedVolumeActor =
        vtkSmartPointer<vtkActor>::New();
    clippedVolumeActor->SetMapper(clippedVolumeMapper);

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

    // Visualization
    // Center camera on image volume.
    vtkSmartPointer<vtkCamera> camera =
        vtkSmartPointer<vtkCamera>::New();
    camera->SetPosition(0, -60, 0);
    camera->SetFocalPoint(center);

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
    volumeRenderer->SetActiveCamera(camera);
    volumeRenderer->SetViewport(volumeViewport);

    volumeRenderer->AddActor(imageVolumeActor);
    volumeRenderer->SetBackground(.1, .2, .3); // Set background color.
    volumeRenderer->RemoveAllLights();

    
    vtkSmartPointer<KeyPressInteractorStyle> style =
        vtkSmartPointer<KeyPressInteractorStyle>::New();
    style->SetCurrentRenderer(volumeRenderer);
    

    vtkSmartPointer<vtkRenderer> axialSliceRenderer =
        vtkSmartPointer<vtkRenderer>::New();
    axialSliceRenderer->SetViewport(axialViewport);
    axialSliceRenderer->AddActor(clippedVolumeActor);
    axialSliceRenderer->SetBackground(.1, .2, .3); // Set background color.

    vtkSmartPointer<vtkRenderer> coronalSliceRenderer =
        vtkSmartPointer<vtkRenderer>::New();
    coronalSliceRenderer->SetViewport(coronalViewport);

    vtkSmartPointer<vtkRenderer> sagittalSliceRenderer =
        vtkSmartPointer<vtkRenderer>::New();
    sagittalSliceRenderer->SetViewport(sagittalViewport);


    renderWindow->AddRenderer(axialSliceRenderer);
    renderWindow->AddRenderer(coronalSliceRenderer);
    renderWindow->AddRenderer(sagittalSliceRenderer);
    renderWindow->AddRenderer(volumeRenderer);


    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
        vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindowInteractor->SetInteractorStyle(style);

    clipPlaneWidget->SetInteractor(renderWindowInteractor);
    clipPlaneWidget->On();


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
