#include "generateStep.hpp"
#include <APIHeaderSection_MakeHeader.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <STEPControl_Writer.hxx>
#include <XSControl_WorkSession.hxx>
#include <TopoDS_Solid.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <gp_Pnt.hxx>
#include <Standard.hxx>
#include <Interface_Static.hxx>


// generate a box as vrml as minimalistic 3D visualization
bool generateStep(const fs::path &path, const std::string &name, const Footprint &footprint) {
    // center of box
    double3 center = footprint.body.offset + double3(footprint.position.x, footprint.position.y, 0);
    center.y = -center.y;

    // size of box
    double3 size = footprint.body.size;

    STEPControl_Writer writer;
    writer.WS()->TransferWriter()->FinderProcess()->Messenger()->ChangePrinters().Clear();

    gp_Pnt p1(center.x - size.x * 0.5, center.y - size.y * 0.5, center.z);
    gp_Pnt p2(center.x + size.x * 0.5, center.y + size.y * 0.5, center.z + size.z);

    // create quader
    BRepPrimAPI_MakeBox boxMaker(p1, p2);//size.x, size.y, size.z);
    TopoDS_Solid box = boxMaker.Solid();  // Oder boxMaker.Shape() für TopoDS_Shape

    // set unit to mm
    Interface_Static::SetCVal("write.step.unit", "MM");

    // add shape to step model
    IFSelect_ReturnStatus status = writer.Transfer(box, STEPControl_AsIs);
    if (status != IFSelect_RetDone) {
        std::cerr << "Error: Transfer of box to step writer failed!" << std::endl;
        return false;
    }

    // set empty date/time
    APIHeaderSection_MakeHeader header(writer.Model());
    header.SetTimeStamp(new TCollection_HAsciiString(""));

    // write step file
    status = writer.Write((path / (name + ".step")).string().c_str());
    if (status != IFSelect_RetDone) {
        std::cerr << "Error: Writing step file failed!" << std::endl;
        return false;
    }
    return true;
}
