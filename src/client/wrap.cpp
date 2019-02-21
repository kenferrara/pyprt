/**
 * Esri CityEngine SDK - Python Bindings
 * 
 * author: Camille Lechot
 */

#include "utils.h"
#include "logging.h"
#include "PyCallbacks.h"

#include "prt/prt.h"
#include "prt/API.h"
#include "prt/ContentType.h"
#include "prt/LogLevel.h"

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>

#include <string>
#include <vector>
#include <iterator>
#include <functional>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>

namespace {

    /**
     * commonly used constants
     */
    const char*    FILE_LOG = "pyprt.log";
    const wchar_t* FILE_CGA_REPORT = L"CGAReport.txt";
    const wchar_t* ENCODER_ID_CGA_REPORT = L"com.esri.prt.core.CGAReportEncoder";
    const wchar_t* ENCODER_ID_PYTHON = L"com.esri.prt.examples.PyEncoder";
    const wchar_t* ENCODER_OPT_NAME = L"name";
    

    /**
     * Helper struct to manage PRT lifetime (e.g. the prt::init() call)
     */
    struct PRTContext {
        PRTContext(prt::LogLevel defaultLogLevel) {
            //const pcu::Path executablePath(pcu::getExecutablePath()); // gives python.exe path (anaconda3 one).
            //std::cout << "PATH GIVEN: " << __FILE__; // gives wrap.cpp path.

            const pcu::Path executablePath("C:\\Users\\cami9495\\Documents\\esri-cityengine-sdk-master\\examples\\py4prt\\install\\bin"); // TO DO: remove the hardcoded path.
            const pcu::Path installPath = executablePath.getParent();
            const pcu::Path fsLogPath = installPath / FILE_LOG;

            //mLogHandler.reset(prt::ConsoleLogHandler::create(prt::LogHandler::ALL, prt::LogHandler::ALL_COUNT));
            mFileLogHandler.reset(prt::FileLogHandler::create(prt::LogHandler::ALL, prt::LogHandler::ALL_COUNT, fsLogPath.native_wstring().c_str()));
            //prt::addLogHandler(mLogHandler.get());
            prt::addLogHandler(mFileLogHandler.get());

            // setup paths for plugins, assume standard SDK layout as per README.md
            const pcu::Path extPath = installPath / "lib";

            // initialize PRT with the path to its extension libraries, the default log level
            const std::wstring wExtPath = extPath.native_wstring();
            const std::array<const wchar_t*, 1> extPaths = { wExtPath.c_str() };
            mPRTHandle.reset(prt::init(extPaths.data(), extPaths.size(), defaultLogLevel));
        }

        ~PRTContext() {
            // shutdown PRT
            mPRTHandle.reset();

            // remove loggers
            //prt::removeLogHandler(mLogHandler.get());
            prt::removeLogHandler(mFileLogHandler.get());
        }

        explicit operator bool() const {
            return (bool)mPRTHandle;
        }

        //pcu::ConsoleLogHandlerPtr mLogHandler;
        pcu::FileLogHandlerPtr    mFileLogHandler;
        pcu::ObjectPtr            mPRTHandle;
    };

} // namespace

PRTContext prtCtx((prt::LogLevel) 0);

namespace {
    class ModelGenerator {
    public:
        ModelGenerator(const std::string& initShapePath, const std::string& rulePkgPath, const std::vector<std::string>& shapeAtt, const std::vector<std::string>& encOpt);
        ~ModelGenerator();
        
        bool generateModel();
        std::vector<std::vector<double>> getModelGeometry() const;
        std::string getModelReport() const;

    private:
        std::string initialShapePath;
        std::string rulePackagePath;
        std::vector<std::string> shapeAttributes;
        std::vector<std::string> encoderOptions;

        std::vector<std::vector<double>> modelGeometry;
        std::string modelReport;

    };

    ModelGenerator::ModelGenerator(const std::string& initShapePath, const std::string& rulePkgPath, const std::vector<std::string>& shapeAtt, const std::vector<std::string>& encOpt) {
        initialShapePath = initShapePath;
        rulePackagePath = rulePkgPath;
        shapeAttributes = shapeAtt;
        encoderOptions = encOpt;
    }

    ModelGenerator::~ModelGenerator() {
    }

    std::vector<std::vector<double>> ModelGenerator::getModelGeometry() const {
        return modelGeometry;
    }

    std::string ModelGenerator::getModelReport() const {
        return modelReport;
    }


    bool ModelGenerator::generateModel() {
        pcu::PyCallbacksPtr foc;
        try {
            // Step 1: Initialization (setup console, logging, licensing information, PRT extension library path, prt::init)
            if (!prtCtx) {
                LOG_ERR << L"failed to get a CityEngine license, bailing out." << std::endl;
            }


            // Step 2: Resolve Map, Callbacks, Cache
            pcu::ResolveMapPtr resolveMap;

            if (!rulePackagePath.empty()) {
                LOG_INF << "Using rule package " << rulePackagePath << std::endl;

                const std::string u8rpkURI = pcu::toFileURI(rulePackagePath);
                prt::Status status = prt::STATUS_UNSPECIFIED_ERROR;
                try {
                    auto* r = prt::createResolveMap(pcu::toUTF16FromUTF8(u8rpkURI).c_str(), nullptr, &status);
                    resolveMap.reset(r);
                }
                catch (std::exception e) {
                    std::cerr << e.what() << std::endl;
                }


                if (resolveMap && (status == prt::STATUS_OK)) {
                    LOG_DBG << "resolve map = " << pcu::objectToXML(resolveMap.get()) << std::endl;
                }
                else {
                    LOG_ERR << "getting resolve map from '" << rulePackagePath << "' failed, aborting." << std::endl;
                }
            }


            // -- create cache & callback
            foc = std::make_unique<PyCallbacks>();
            pcu::CachePtr cache{ prt::CacheObject::create(prt::CacheObject::CACHE_TYPE_DEFAULT) };


            // Step 3: Initial Shape
            pcu::InitialShapeBuilderPtr isb{ prt::InitialShapeBuilder::create() };
            if (!pcu::toFileURI(initialShapePath).empty()) {
                LOG_DBG << L"trying to read initial shape geometry from " << pcu::toFileURI(initialShapePath);
                const prt::Status s = isb->resolveGeometry(pcu::toUTF16FromOSNarrow(pcu::toFileURI(initialShapePath)).c_str(), resolveMap.get(), cache.get());
                if (s != prt::STATUS_OK) {
                    LOG_ERR << "could not resolve geometry from " << pcu::toFileURI(initialShapePath);
                }
            }
            else {
                isb->setGeometry(
                    pcu::quad::vertices, pcu::quad::vertexCount,
                    pcu::quad::indices, pcu::quad::indexCount,
                    pcu::quad::faceCounts, pcu::quad::faceCountsCount
                );
            }


            // -- setup initial shape attributes
            std::wstring       ruleFile = L"bin/rule.cgb";
            std::wstring       startRule = L"default$init";
            int32_t            seed = 666;
            const std::wstring shapeName = L"TheInitialShape";

            const pcu::AttributeMapPtr convertedShapeAttr{ pcu::createAttributeMapFromTypedKeyValues(shapeAttributes) };
            if (convertedShapeAttr) {
                if (convertedShapeAttr->hasKey(L"ruleFile") &&
                    convertedShapeAttr->getType(L"ruleFile") == prt::AttributeMap::PT_STRING)
                    ruleFile = convertedShapeAttr->getString(L"ruleFile");
                if (convertedShapeAttr->hasKey(L"startRule") &&
                    convertedShapeAttr->getType(L"startRule") == prt::AttributeMap::PT_STRING)
                    startRule = convertedShapeAttr->getString(L"startRule");
            }


            isb->setAttributes(
                ruleFile.c_str(),
                startRule.c_str(),
                seed,
                shapeName.c_str(),
                convertedShapeAttr.get(),
                resolveMap.get()
            );


            // -- create initial shape
            const pcu::InitialShapePtr initialShape{ isb->createInitialShapeAndReset() };
            const std::vector<const prt::InitialShape*> initialShapes = { initialShape.get() };


            // Step 4 : Generate (encoder info, encoder options, trigger procedural 3D model generation)
            const pcu::AttributeMapBuilderPtr optionsBuilder{ prt::AttributeMapBuilder::create() };
            optionsBuilder->setString(ENCODER_OPT_NAME, FILE_CGA_REPORT);
            const pcu::AttributeMapPtr reportOptions{ optionsBuilder->createAttributeMapAndReset() };
            const pcu::AttributeMapPtr encOptions{ pcu::createAttributeMapFromTypedKeyValues(encoderOptions) };


            // -- validate & complete encoder options
            const pcu::AttributeMapPtr validatedEncOpts{ createValidatedOptions(ENCODER_ID_PYTHON, encOptions) };
            const pcu::AttributeMapPtr validatedReportOpts{ createValidatedOptions(ENCODER_ID_CGA_REPORT, reportOptions) };


            // -- setup encoder IDs and corresponding options
            const std::array<const wchar_t*, 2> encoders = {
                    ENCODER_ID_PYTHON,     // Python geometry encoder
                    ENCODER_ID_CGA_REPORT, // an encoder to redirect CGA report to CGAReport.txt
            };
            const std::array<const prt::AttributeMap*, 2> encoderOpts = { validatedEncOpts.get(), validatedReportOpts.get() };
        


            // Step 5: Generate
            const prt::Status genStat = prt::generate(
                initialShapes.data(), initialShapes.size(), nullptr,
                encoders.data(), encoders.size(), encoderOpts.data(),
                foc.get(), cache.get(), nullptr
            );

            if (genStat != prt::STATUS_OK) {
                LOG_ERR << "prt::generate() failed with status: '" << prt::getStatusDescription(genStat) << "' (" << genStat << ")";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "caught exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "caught unknown exception." << std::endl;
        }
    
        modelGeometry = foc->getGeometry();
        modelReport = foc->getReport();
        return true;
    }

} // namespace

int py_printVal(int val) {
    return val;
}

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(pyprt, m) {
    py::class_<ModelGenerator>(m, "ModelGenerator")
        .def(py::init<const std::string&,const std::string&,const std::vector<std::string>&,const std::vector<std::string>&>(), "initShapePath"_a, "rulePkgPath"_a, "shapeAtt"_a, "encOpt"_a)
        .def("generate_model", &ModelGenerator::generateModel)
        .def("get_model_geometry", &ModelGenerator::getModelGeometry)
        .def("get_model_report", &ModelGenerator::getModelReport);

    m.def("print_val",&py_printVal,"Test Python function for value printing.");
}