#include "CameraFixtures.h"

#include <fstream>
#include <sstream>

#include <QFile>
#include <QString>
#include <QTextStream>

#include "AlphaCube.h"
#include "Brick.h"
#include "Cube.h"
#include "FileName.h"
#include "LineManager.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"

using json = nlohmann::json;

namespace Isis {

  void PushFramePair::SetUp() {
    numSamps = 16;
    numBands = 3;
    frameHeight = 12;
    numFrames = 10;

    evenCube.reset(new Cube());
    evenCube->setDimensions(numSamps, frameHeight * numFrames, numBands);
    evenCube->create(tempDir.path() + "/even.cub");

    oddCube.reset(new Cube());
    oddCube->setDimensions(numSamps, frameHeight * numFrames, numBands);
    oddCube->create(tempDir.path() + "/odd.cub");

    Brick frameBrick(numSamps, frameHeight, numBands, evenCube->pixelType());

    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
      for (int brickIndex = 0; brickIndex < frameBrick.size(); brickIndex++) {
        frameBrick[brickIndex] = frameIndex + 1;
      }
      frameBrick.SetBasePosition(1,frameIndex * frameHeight + 1,1);
      if (frameIndex % 2 == 0) {
        oddCube->write(frameBrick);
      }
      else {
        evenCube->write(frameBrick);
      }
    }

    PvlGroup intGroup("Instrument");
    intGroup += PvlKeyword("StartTime", "2008-06-14T13:32:10.933207");
    evenCube->putGroup(intGroup);
    oddCube->putGroup(intGroup);

    evenCube->reopen("rw");
    oddCube->reopen("rw");

  }


  void FlippedPushFramePair::SetUp() {
    numSamps = 16;
    numBands = 3;
    frameHeight = 12;
    numFrames = 10;

    evenCube.reset(new Cube());
    evenCube->setDimensions(numSamps, frameHeight * numFrames, numBands);
    evenCube->create(tempDir.path() + "/even.cub");

    oddCube.reset(new Cube());
    oddCube->setDimensions(numSamps, frameHeight * numFrames, numBands);
    oddCube->create(tempDir.path() + "/odd.cub");

    Brick frameBrick(numSamps, frameHeight, numBands, evenCube->pixelType());

    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
      for (int brickIndex = 0; brickIndex < frameBrick.size(); brickIndex++) {
        frameBrick[brickIndex] = numFrames - frameIndex;
      }
      frameBrick.SetBasePosition(1,frameIndex * frameHeight + 1,1);
      if (frameIndex % 2 == 0) {
        evenCube->write(frameBrick);
      }
      else {
        oddCube->write(frameBrick);
      }
    }

    PvlGroup intGroup("Instrument");
    intGroup += PvlKeyword("DataFlipped", "True");
    intGroup += PvlKeyword("StartTime", "2008-06-14T13:32:10.933207");
    evenCube->putGroup(intGroup);
    oddCube->putGroup(intGroup);

    evenCube->reopen("rw");
    oddCube->reopen("rw");

  }


  void DemCube::SetUp() {
    DefaultCube::SetUp();
    testCube->label()->object(4)["SolarLongitude"] = "294.73518831328";
    testCube->reopen("rw");

    std::ifstream cubeLabel("data/defaultImage/demCube.pvl");

    Pvl demLabel;
    cubeLabel >> demLabel;
    demLabel.findObject("IsisCube").findObject("Core").findGroup("Pixels")["Type"] = "Real";

    demCube = new Cube();
    demCube->fromLabel(tempDir.path() + "/demCube.cub", demLabel, "rw");

    TableField minRadius("MinimumRadius", TableField::Double);
    TableField maxRadius("MaximumRadius", TableField::Double);

    TableRecord record;
    record += minRadius;
    record += maxRadius;

    Table shapeModelStatistics("ShapeModelStatistics", record);

    record[0] = 3376.2;
    record[1] = 3396.19;
    shapeModelStatistics += record;

    demCube->write(shapeModelStatistics);

    int xCenter = int(demCube->lineCount()/2);
    int yCenter = int(demCube->sampleCount()/2);
    double radius = std::min(xCenter, yCenter);
    double depth = 30;
    double pointRadius;

    LineManager line(*demCube);
    double pixelValue;
    double base = demCube->label()->findObject("IsisCube").findObject("Core").findGroup("Pixels")["Base"];
    double xPos = 0.0;

    for(line.begin(); !line.end(); line++) {
      for(int yPos = 0; yPos < line.size(); yPos++) {
        pointRadius = pow(pow((xPos - xCenter), 2) + pow((yPos - yCenter), 2), 0.5);
        if (pointRadius < radius) {
          pixelValue = ((sin(((M_PI*pointRadius)/(2*radius))) * depth) + depth) + base;
        }
        else {
          pixelValue = base + (depth * 2);
        }
        line[yPos] = (double) pixelValue;
      }
      xPos++;
      demCube->write(line);
    }

    demCube->reopen("rw");
  }


  void DemCube::TearDown() {
    if (demCube->isOpen()) {
      demCube->close();
    }

    delete demCube;
  }


  void DefaultCube::SetUp() {
    TempTestingFiles::SetUp();

    std::ifstream isdFile("data/defaultImage/defaultCube.isd");
    std::ifstream cubeLabel("data/defaultImage/defaultCube.pvl");
    std::ifstream projCubeLabel("data/defaultImage/projDefaultCube.pvl");

    isdFile >> isd;
    cubeLabel >> label;
    projCubeLabel >> projLabel;

    testCube = new Cube();
    testCube->fromIsd(tempDir.path() + "/default.cub", label, isd, "rw");

    LineManager line(*testCube);
    int pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      testCube->write(line);
    }

    projTestCube = new Cube();
    projTestCube->fromIsd(tempDir.path() + "/default.level2.cub", projLabel, isd, "rw");

    line = LineManager(*projTestCube);
    pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      projTestCube->write(line);
    }
    projTestCube->reopen("rw");
  }


  void DefaultCube::resizeCube(int samples, int lines, int bands) {
    label = Pvl();
    PvlObject &isisCube = testCube->label()->findObject("IsisCube");
    label.addObject(isisCube);

    PvlGroup &dim = label.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    dim.findKeyword("Samples").setValue(QString::number(samples));
    dim.findKeyword("Lines").setValue(QString::number(lines));
    dim.findKeyword("Bands").setValue(QString::number(bands));

    delete testCube;
    testCube = new Cube();
    testCube->fromIsd(tempDir.path() + "/default.cub", label, isd, "rw");

    LineManager line(*testCube);
    int pixelValue = 1;
    for(int band = 1; band <= bands; band++) {
      for (int i = 1; i <= testCube->lineCount(); i++) {
        line.SetLine(i, band);
        for (int j = 0; j < line.size(); j++) {
          line[j] = (double) (pixelValue % 255);
          pixelValue++;
        }
        testCube->write(line);
      }
    }

    projLabel = Pvl();
    PvlObject &isisProjCube= projTestCube->label()->findObject("IsisCube");
    projLabel.addObject(isisProjCube);

    PvlGroup &projDim = projLabel.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    projDim.findKeyword("Samples").setValue(QString::number(samples));
    projDim.findKeyword("Lines").setValue(QString::number(lines));
    projDim.findKeyword("Bands").setValue(QString::number(bands));

    delete projTestCube;
    projTestCube = new Cube();
    projTestCube->fromIsd(tempDir.path() + "/default.level2.cub", projLabel, isd, "rw");

    line = LineManager(*projTestCube);
    pixelValue = 1;
    for(int band = 1; band <= bands; band++) {
      for (int i = 1; i <= projTestCube->lineCount(); i++) {
        line.SetLine(i, band);
        for (int j = 0; j < line.size(); j++) {
          line[j] = (double) (pixelValue % 255);
          pixelValue++;
        }
        projTestCube->write(line);
      }
    }
  }


  void DefaultCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (projTestCube->isOpen()) {
      projTestCube->close();
    }

    delete testCube;
    delete projTestCube;
  }


  void LineScannerCube::SetUp() {
    TempTestingFiles::SetUp();

    std::ifstream isdFile("data/LineScannerImage/defaultLineScanner.isd");
    std::ifstream cubeLabel("data/LineScannerImage/defaultLineScanner.pvl");
    std::ifstream projCubeLabel("data/LineScannerImage/projDefaultLineScanner.pvl");

    isdFile >> isd;
    cubeLabel >> label;
    projCubeLabel >> projLabel;

    testCube = new Cube();
    testCube->fromIsd(tempDir.path() + "/default.cub", label, isd, "rw");
    LineManager line(*testCube);
    int pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      testCube->write(line);
    }

    projTestCube = new Cube();
    projTestCube->fromIsd(tempDir.path() + "/default.level2.cub", projLabel, isd, "rw");
    line = LineManager(*projTestCube);
    pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      projTestCube->write(line);
    }
  }


  void LineScannerCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (projTestCube->isOpen()) {
      projTestCube->close();
    }

    delete testCube;
    delete projTestCube;
  }


  void OffBodyCube::SetUp() {
    TempTestingFiles::SetUp();
    testCube = new Cube("data/offBodyImage/EW0131773041G.cal.crop.cub");
  }


  void OffBodyCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    delete testCube;
  }


  void MiniRFCube::SetUp() {
    TempTestingFiles::SetUp();
    testCube = new Cube("data/miniRFImage/LSZ_04866_1CD_XKU_89N109_V1_lev1.crop.cub");
  }


  void MiniRFCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    delete testCube;
  }


  void MroCtxCube::SetUp() {
    TempTestingFiles::SetUp();

    QString testPath = tempDir.path() + "/test.cub";
    QFile::copy("data/mroCtxImage/ctxTestImage.cub", testPath);
    testCube.reset(new Cube(testPath));
  }


  void MroCtxCube::TearDown() {
    testCube.reset();
  }


  void GalileoSsiCube::SetUp() {
    DefaultCube::SetUp();

    // Change default dims
    PvlGroup &dim = label.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    dim.findKeyword("Samples").setValue("800");
    dim.findKeyword("Lines").setValue("800");
    dim.findKeyword("Bands").setValue("1");

    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");
    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue("-77001");
    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");

    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName            = "Galileo Orbiter"
        InstrumentId              = "SOLID STATE IMAGING SYSTEM"
        TargetName                = IO
        SpacecraftClockStartCount = 05208734.39
        StartTime                 = 1999-10-11T18:05:15.815
        ExposureDuration          = 0.04583 <seconds>
        GainModeId                = 100000
        TelemetryFormat           = IM4
        LightFloodStateFlag       = ON
        InvertedClockStateFlag    = "NOT INVERTED"
        BlemishProtectionFlag     = OFF
        ExposureType              = NORMAL
        ReadoutMode               = Contiguous
        FrameDuration             = 8.667 <seconds>
        Summing                   = 1
        FrameModeId               = FULL
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;
    inst = newInstGroup;

    PvlGroup &bandBin = testCube->label()->findObject("IsisCube").findGroup("BandBin");
    std::istringstream bss(R"(
      Group = BandBin
        FilterName   = RED
        FilterNumber = 2
        Center       = 0.671 <micrometers>
        Width        = .06 <micrometers>
      End_Group
    )");

    PvlGroup newBandBin;
    bss >> newBandBin;
    bandBin = newBandBin;

    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");

    std::istringstream nk(R"(
      Object = NaifKeywords
        BODY_CODE                  = 501
        BODY501_RADII              = (1829.4, 1819.3, 1815.7)
        BODY_FRAME_CODE            = 10023
        INS-77001_FOCAL_LENGTH     = 1500.46655964
        INS-77001_K1               = -2.4976983626e-05
        INS-77001_PIXEL_PITCH      = 0.01524
        INS-77001_TRANSX           = (0.0, 0.01524, 0.0)
        INS-77001_TRANSY           = (0.0, 0.0, 0.01524)
        INS-77001_ITRANSS          = (0.0, 65.6167979, 0.0)
        INS-77001_ITRANSL          = (0.0, 0.0, 65.6167979)
        INS-77001_BORESIGHT_SAMPLE = 400.0
        INS-77001_BORESIGHT_LINE   = 400.0
      End_Object
    )");

    PvlObject newNaifKeywords;
    nk >> newNaifKeywords;
    naifKeywords = newNaifKeywords;

    std::istringstream ar(R"(
    Group = Archive
      DataSetId     = GO-J/JSA-SSI-2-REDR-V1.0
      ProductId     = 24I0146
      ObservationId = 24ISGLOCOL01
      DataType      = RADIANCE
      CalTargetCode = 24
    End_Group
    )");

    PvlGroup &archive = testCube->label()->findObject("IsisCube").findGroup("Archive");
    PvlGroup newArchive;
    ar >> newArchive;
    archive = newArchive;

    LineManager line(*testCube);
    for(line.begin(); !line.end(); line++) {
        for(int i = 0; i < line.size(); i++) {
          line[i] = (double)(i+1);
        }
        testCube->write(line);
    }

    // need to remove old camera pointer
    delete testCube;
    testCube = new Cube(newCube, "rw");
  }


  void GalileoSsiCube::TearDown() {
    if (testCube) {
      delete testCube;
    }
  }


  void MroHiriseCube::SetUp() {
    DefaultCube::SetUp();
    dejitteredCube.open("data/mroKernels/mroHiriseProj.cub");

    // force real DNs
    QString fname = testCube->fileName();

    PvlObject &core = label.findObject("IsisCube").findObject("Core");
    PvlGroup &pixels = core.findGroup("Pixels");
    pixels.findKeyword("Type").setValue("Real");

    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");
    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue("-74999");
    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName              = "MARS RECONNAISSANCE ORBITER"
        InstrumentId                = HIRISE
        TargetName                  = Mars
        StartTime                   = 2008-05-17T09:37:24.7300819
        StopTime                    = 2008-05-17T09:37:31.0666673
        ObservationStartCount       = 895484264:44383
        SpacecraftClockStartCount   = 895484264:57342
        SpacecraftClockStopCount    = 895484272:12777
        ReadoutStartCount           = 895484659:31935
        CalibrationStartTime        = 2006-11-08T04:49:13.952
        CalibrationStartCount       = 847428572:51413
        AnalogPowerStartTime        = 2006-11-08T04:48:34.478
        AnalogPowerStartCount       = 847428533:20297
        MissionPhaseName            = "PRIMARY SCIENCE PHASE"
        LineExposureDuration        = 95.0625 <MICROSECONDS>
        ScanExposureDuration        = 95.0625 <MICROSECONDS>
        DeltaLineTimerCount         = 337
        Summing                     = 1
        Tdi                         = 128
        FocusPositionCount          = 2020
        PoweredCpmmFlag             = (On, On, On, On, On, On, On, On, On, On, On,
                                      On, On, On)
        CpmmNumber                  = 8
        CcdId                       = RED5
        ChannelNumber               = 0
        LookupTableType             = Stored
        LookupTableNumber           = 19
        LookupTableMinimum          = -9998
        LookupTableMaximum          = -9998
        LookupTableMedian           = -9998
        LookupTableKValue           = -9998
        StimulationLampFlag         = (Off, Off, Off)
        HeaterControlFlag           = (On, On, On, On, On, On, On, On, On, On, On,
                                      On, On, On)
        OptBnchFlexureTemperature   = 19.5881 <C>
        OptBnchMirrorTemperature    = 19.6748 <C>
        OptBnchFoldFlatTemperature  = 19.9348 <C>
        OptBnchFpaTemperature       = 19.5015 <C>
        OptBnchFpeTemperature       = 19.2415 <C>
        OptBnchLivingRmTemperature  = 19.4148 <C>
        OptBnchBoxBeamTemperature   = 19.5881 <C>
        OptBnchCoverTemperature     = 19.6748 <C>
        FieldStopTemperature        = 17.9418 <C>
        FpaPositiveYTemperature     = 18.8082 <C>
        FpaNegativeYTemperature     = 18.6349 <C>
        FpeTemperature              = 18.0284 <C>
        PrimaryMirrorMntTemperature = 19.5015 <C>
        PrimaryMirrorTemperature    = 19.6748 <C>
        PrimaryMirrorBafTemperature = 2.39402 <C>
        MsTrussLeg0ATemperature     = 19.6748 <C>
        MsTrussLeg0BTemperature     = 19.8482 <C>
        MsTrussLeg120ATemperature   = 19.3281 <C>
        MsTrussLeg120BTemperature   = 20.1949 <C>
        MsTrussLeg240ATemperature   = 20.2816 <C>
        MsTrussLeg240BTemperature   = 20.7151 <C>
        BarrelBaffleTemperature     = -13.8299 <C>
        SunShadeTemperature         = -33.9377 <C>
        SpiderLeg30Temperature      = 17.5087 <C>
        SpiderLeg120Temperature     = -9999
        SpiderLeg240Temperature     = -9999
        SecMirrorMtrRngTemperature  = 20.6284 <C>
        SecMirrorTemperature        = 20.455 <C>
        SecMirrorBaffleTemperature  = -11.1761 <C>
        IeaTemperature              = 25.4878 <C>
        FocusMotorTemperature       = 21.4088 <C>
        IePwsBoardTemperature       = 16.3696 <C>
        CpmmPwsBoardTemperature     = 17.6224 <C>
        MechTlmBoardTemperature     = 34.7792 <C>
        InstContBoardTemperature    = 34.4121 <C>
        DllLockedFlag               = (YES, YES)
        DllResetCount               = 0
        DllLockedOnceFlag           = (YES, YES)
        DllFrequenceCorrectCount    = 4
        ADCTimingSetting            = -9999
        Unlutted                    = TRUE
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;

    newInstGroup.findKeyword("InstrumentId").setValue("HIRISE");
    newInstGroup.findKeyword("SpacecraftName").setValue("MARS RECONNAISSANCE ORBITER");

    inst = newInstGroup;
    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");

    PvlKeyword startcc("SpacecraftClockStartCount", "33322515");
    PvlKeyword stopcc("SpaceCraftClockStopCount", "33322516");
    inst += startcc;
    inst += stopcc;

    json nk;
    nk["INS-74999_FOCAL_LENGTH"] = 11994.9988;
    nk["INS-74999_PIXEL_PITCH"] = 0.012;
    nk["INS-74605_TRANSX"] = {-89.496, -1.0e-06, 0.012};
    nk["INS-74605_TRANSY"] = {-12.001, -0.012, -1.0e-06};
    nk["INS-74605_ITRANSS"] = {-1000.86, -0.0087, -83.333};
    nk["INS-74605_ITRANSL"] = {7457.9, 83.3333, -0.0087};
    nk["INS-74999_OD_K"] = {-0.0048509, 2.41312e-07, -1.62369e-13};
    nk["BODY499_RADII"] = {3396.19, 3396.19, 3376.2};
    nk["CLOCK_ET_-74999_895484264:57342_COMPUTED"] = "8ed6ae8930f3bd41";

    nk["BODY_CODE"] = 499;
    nk["BODY_FRAME_CODE"] = 10014;
    PvlObject newNaifKeywords("NaifKeywords", nk);
    naifKeywords = newNaifKeywords;

    QString fileName = testCube->fileName();

    LineManager line(*testCube);
    for(line.begin(); !line.end(); line++) {
        for(int i = 0; i < line.size(); i++) {
          line[i] = (double)(i+1);
        }
        testCube->write(line);
    }
    testCube->reopen("rw");

    // need to remove old camera pointer
    delete testCube;
    // This is now a MRO cube

    testCube = new Cube(fileName, "rw");

    // create a jitter file
    QString jitter = R"(# Sample                 Line                   ET
-0.18     -0.07     264289109.96933
-0.11     -0.04     264289109.97
-0.05     -0.02     264289109.98
1.5     0.6     264289110.06
    )";

    jitterPath = tempDir.path() + "/jitter.txt";
    QFile jitterFile(jitterPath);

    if (jitterFile.open(QIODevice::WriteOnly)) {
      QTextStream out(&jitterFile);
      out << jitter;
      jitterFile.close();
    }
    else {
      FAIL() << "Failed to create Jitter file" << std::endl;
    }
  }


  void NewHorizonsCube::setInstrument(QString ikid, QString instrumentId, QString spacecraftName) {
    PvlObject &isisCube = testCube->label()->findObject("IsisCube");

    label = Pvl();
    label.addObject(isisCube);

    PvlGroup &kernels = label.findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue(ikid);
    kernels["ShapeModel"] = "Null";

    PvlGroup &dim = label.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    dim.findKeyword("Samples").setValue("10");
    dim.findKeyword("Lines").setValue("10");
    dim.findKeyword("Bands").setValue("2");

    PvlGroup &pixels = label.findObject("IsisCube").findObject("Core").findGroup("Pixels");
    pixels.findKeyword("Type").setValue("Real");

    PvlGroup &inst = label.findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName            = "NEW HORIZONS"
        InstrumentId              = LEISA
        TargetName                = Jupiter
        SpacecraftClockStartCount = 1/0034933739:00000
        ExposureDuration          = 0.349
        StartTime                 = 2007-02-28T01:57:01.3882862
        StopTime                  = 2007-02-28T02:04:53.3882861
        FrameRate                 = 2.86533 <Hz>
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;

    newInstGroup.findKeyword("InstrumentId").setValue(instrumentId);
    newInstGroup.findKeyword("SpacecraftName").setValue(spacecraftName);

    inst = newInstGroup;

    PvlGroup &bandBin = label.findObject("IsisCube").findGroup("BandBin");
    std::istringstream bss(R"(
      Group = BandBin
        Center       = (2.4892, 1.2204)
        Width        = (0.011228, 0.005505)
        OriginalBand = (1, 200)
      End_Group
    )");

    PvlGroup newBandBin;
    bss >> newBandBin;
    bandBin = newBandBin;

    std::istringstream alphaSS(R"(
      Group = AlphaCube
        AlphaSamples        = 256
        AlphaLines          = 1354
        AlphaStartingSample = 0.5
        AlphaStartingLine   = 229.5
        AlphaEndingSample   = 100.5
        AlphaEndingLine     = 329.5
        BetaSamples         = 100
        BetaLines           = 100
      End_Group
    )");

    PvlGroup alphaGroup;
    alphaSS >> alphaGroup;
    label.findObject("IsisCube").addGroup(alphaGroup);

    std::ifstream isdFile("data/leisa/nh_leisa.isd");
    isdFile >> isd;

    QString fileName = tempDir.path() + "/leisa.cub";
    delete testCube;
    testCube = new Cube();
    testCube->fromIsd(fileName, label, isd, "rw");

    LineManager line(*testCube);
    double pixelValue = 0.0;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) pixelValue++;
      }
      testCube->write(line);
    }
  }


  void OsirisRexCube::setInstrument(QString ikid, QString instrumentId) {
    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");

    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue(ikid);
    kernels["ShapeModel"] = "Null";

    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        MissionName               = OSIRIS-REx
        SpacecraftName            = OSIRIS-REX
        InstrumentId              = PolyCam
        TargetName                = Bennu
        StartTime                 = 2019-01-13T23:36:05.000
        ExposureDuration          = 100 <ms>
        SpacecraftClockStartCount = 1/0600694569.00000
        FocusPosition             = 21510
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;

    newInstGroup.findKeyword("InstrumentId").setValue(instrumentId);

    inst = newInstGroup;

    PvlGroup &bandBin = label.findObject("IsisCube").findGroup("BandBin");
    std::istringstream bss(R"(
      Group = BandBin
        FilterName = Unknown
      End_Group
    )");

    PvlGroup newBandBin;
    bss >> newBandBin;
    bandBin = newBandBin;

    json nk;
    nk["BODY2101955_RADII"] =  {2825, 2675, 254};
    nk["INS"+ikid.toStdString()+"_FOCAL_LENGTH"] = 630.0;
    nk["INS"+ikid.toStdString()+"_PIXEL_SIZE"] = 8.5;
    nk["CLOCK_ET_-64_1/0600694569.00000_COMPUTED"] = "8ed6ae8930f3bd41";
    nk["INS"+ikid.toStdString()+"_TRANSX"] = {0.0, 0.0085, 0.0};
    nk["INS"+ikid.toStdString()+"_TRANSY"] = {0.0, 0.0, -0.0085};
    nk["INS"+ikid.toStdString()+"_ITRANSS"] = {0.0, 117.64705882353, 0.0};
    nk["INS"+ikid.toStdString()+"_ITRANSL"] = {0.0, 0.0, -117.64705882353};
    nk["INS"+ikid.toStdString()+"_CCD_CENTER"] = {511.5, 511.5};
    nk["BODY_FRAME_CODE"] = 2101955;

    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");
    PvlObject newNaifKeywords("NaifKeywords", nk);
    naifKeywords = newNaifKeywords;

    QString fileName = testCube->fileName();
    delete testCube;
    testCube = new Cube(fileName, "rw");
  }


  void MgsMocCube::SetUp() {
    TempTestingFiles::SetUp();

    QString testPath = tempDir.path() + "/test.cub";
    QFile::copy("data/mgsImages/mocImage.cub", testPath);
    testCube.reset(new Cube(testPath));
  }


  void MgsMocCube::TearDown() {
    testCube.reset();
  }

  void ClipperWacFcCube::SetUp() {
    TempTestingFiles::SetUp();

    QString testPath = tempDir.path() + "/test.cub";
    QFile::copy("data/clipper/ClipperWacFc.cub", testPath);
    wacFcCube = new Cube(testPath);

    PvlGroup &wacKernels = wacFcCube->label()->findObject("IsisCube").findGroup("Kernels");
    wacKernels.findKeyword("NaifFrameCode").setValue("-159102");

    double offset = 10;
    AlphaCube aCube(wacFcCube->sampleCount(), wacFcCube->lineCount(),
                    wacFcCube->sampleCount()-offset, wacFcCube->lineCount() - offset,
                    0, offset, wacFcCube->sampleCount(), wacFcCube->lineCount());

    aCube.UpdateGroup(*wacFcCube);

    wacFcCube->reopen("rw");
  }

  void ClipperWacFcCube::TearDown() {
    if (wacFcCube) {
      delete wacFcCube;
    }
  }

  void ClipperNacRsCube::SetUp() {
    DefaultCube::SetUp();

    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");

    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue("-159101");

    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName            = Clipper
        InstrumentId              = EIS-NAC-RS
        TargetName                = Europa
        StartTime                 = 2025-01-01T00:00:00.000
        JitterSampleCoefficients = (0.0, 0.0, 0.0)
        JitterLineCoefficients   = (0.0, 0.0, 0.0)
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;
    inst = newInstGroup;

    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");
    std::istringstream nk(R"(
      Object = NaifKeywords
        BODY_CODE               = 502
        BODY502_RADII           = (1562.6, 1560.3, 1559.5)
        BODY_FRAME_CODE         = 10024
        INS-159101_FOCAL_LENGTH = 150.40199
        INS-159101_PIXEL_PITCH  = 0.014
        INS-159101_TRANSX       = (0.0, 0.014004651, 0.0)
        INS-159101_TRANSY       = (0.0, 0.0, 0.01399535)
        INS-159101_ITRANSS      = (0.0, 71.404849, 0.0)
        INS-159101_ITRANSL      = (0.0, 0.0, 71.4523)
        INS-159101_OD_K         = (0.0, 0.0, 0.0)
      End_Object
    )");

    PvlObject newNaifKeywords;
    nk >> newNaifKeywords;
    naifKeywords = newNaifKeywords;

    QString fileName = testCube->fileName();
    delete testCube;
    testCube = new Cube(fileName, "rw");

    double offset = 10;
    AlphaCube aCube(testCube->sampleCount(), testCube->lineCount(),
                    testCube->sampleCount()-offset, testCube->lineCount() - offset,
                    0, offset, testCube->sampleCount(), testCube->lineCount());

    aCube.UpdateGroup(*testCube);
    testCube->reopen("rw");
  }

  void ClipperNacRsCube::TearDown() {
    if (testCube) {
      delete testCube;
    }
  }

  void ClipperPbCube::setInstrument(QString instrumentId) {
    TempTestingFiles::SetUp();

    if (instrumentId == "EIS-NAC-PB") {
      QString testPath = tempDir.path() + "/nacTest.cub";
      QFile::copy("data/clipper/ClipperNacPb.cub", testPath);
      testCube = new Cube(testPath, "rw");
    }
    else if (instrumentId == "EIS-WAC-PB") {
      QString testPath = tempDir.path() + "/wacTest.cub";
      QFile::copy("data/clipper/ClipperWacPb.cub", testPath);
      testCube = new Cube(testPath, "rw");
    }
  }

  void NearMsiCameraCube::SetUp() {
    TempTestingFiles::SetUp();

    json isd;
    Pvl label;

    std::ifstream isdFile("data/near/msicamera/m0155881376f3_2p_cif_dbl.isd");
    std::ifstream cubeLabel("data/near/msicamera/m0155881376f3_2p_cif_dbl.pvl");

    isdFile >> isd;
    cubeLabel >> label;

    testCube.reset( new Cube() ) ;
    testCube->fromIsd(tempDir.path() + "/m0155881376f3_2p_cif_dbl.cub", label, isd, "rw");
  }

  void NearMsiCameraCube::TearDown() {
    testCube.reset();
  }

}