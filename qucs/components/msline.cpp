/***************************************************************************
                          msline.cpp  -  description
                             -------------------
    begin                : Sat Aug 23 2003
    copyright            : (C) 2003 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "node.h"
#include <QMap>
#include <QPair>
#include "msline.h"
#include "extsimkernels/spicecompat.h"


MSline::MSline()
{
  Description = QObject::tr("microstrip line");
  Simulator = spicecompat::simQucsator + spicecompat::simNgspice;

  Lines.append(new qucs::Line(-30,  0,-18,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 18,  0, 30,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-13, -8, 23, -8,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-23,  8, 13,  8,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-13, -8,-23,  8,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 23, -8, 13,  8,QPen(Qt::darkBlue,2)));

  Ports.append(new Port(-30, 0));
  Ports.append(new Port( 30, 0));

  x1 = -30; y1 =-11;
  x2 =  30; y2 = 11;

  tx = x1+4;
  ty = y2+4;
  Model = "MLIN";
  Name  = "MS";
  SpiceModel = "A";

  Props.append(new Property("Subst", "Subst1", true,
	QObject::tr("name of substrate definition")));
  Props.append(new Property("W", "1 mm", true,
	QObject::tr("width of the line")));
  Props.append(new Property("L", "10 mm", true,
	QObject::tr("length of the line")));
  Props.append(new Property("Model", "Hammerstad", false,
	QObject::tr("quasi-static microstrip model")+
		    " [Hammerstad, Wheeler, Schneider]"));
  Props.append(new Property("Type", "Standard", false,
    QObject::tr("Microstrip line type") + " [Standard,Embedded]"));
  Props.append(new Property("DispModel", "Kirschning", false,
	QObject::tr("microstrip dispersion model")+" [Kirschning, Kobayashi, "
	"Yamashita, Hammerstad, Getsinger, Schneider, Pramanick]"));
  Props.append(new Property("TopMetal", "TopMetal2", true,
    QObject::tr("Top metal layer") + " [TopMetal2,TopMetal1,Metal5,Metal4,Metal3,Metal2]"));
  Props.append(new Property("BottomMetal", "TopMetal1", true,
    QObject::tr("Bottom metal layer") + " [TopMetal1,Metal5,Metal4,Metal3,Metal2,Metal1]"));
  Props.append(new Property("Temp", "26.85", false,
	QObject::tr("simulation temperature in degree Celsius")));
  Props.append(new Property("TranModel", "DC", false,
                            QObject::tr("Transisent model") + " [DC,Full]"));
  getProperty("TranModel")->simulators = spicecompat::simNgspice;
}

MSline::~MSline()
{
}

Component* MSline::newOne()
{
  return new MSline();
}

Element* MSline::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Microstrip Line");
  BitmapFile = (char *) "msline";

  if(getNewOne)  return new MSline();
  return 0;
}

QString MSline::spice_netlist(spicecompat::SpiceDialect dialect)
{
  QString s;
  if (dialect != spicecompat::SPICEDefault) return s;
  QString subline = getSpiceSubstrateLine();
  QString p1 = spicecompat::normalize_node_name(Ports.at(0)->Connection->Name);
  QString p2 = spicecompat::normalize_node_name(Ports.at(1)->Connection->Name);

  QString L = spicecompat::normalize_value(getProperty("L")->Value);
  QString W = spicecompat::normalize_value(getProperty("W")->Value);

  int Mod = spicecompat::strToMSlineModel(getProperty("Model")->Value);
  int Disp = spicecompat::strToDispModel(getProperty("DispModel")->Value);
  int Tran = spicecompat::strToTranModel(getProperty("TranModel")->Value);

  QString hammerstadParams = "";
  if (getProperty("Type")->Value == "Embedded") {
      QString topMetal = getProperty("TopMetal")->Value;
      QString bottomMetal = getProperty("BottomMetal")->Value;

      QString top = topMetal.replace("TopMetal", "TM").replace("Metal", "M");
      QString bottom = bottomMetal.replace("TopMetal", "TM").replace("Metal", "M");

      double h1, h2, t_embed;
      getHammerstadValues(top, bottom, h1, h2, t_embed);
      hammerstadParams = QString("type=1 h1=%1e-9 h2=%2e-9 t_embed=%3e-9")
          .arg(h1).arg(h2).arg(t_embed);
  }

  s = QString("A_%1 %hd(%2 0) %hd(%3 0) %vd(%2 0) %vd(%3 0) MODEL_%1\n")
          .arg(Name).arg(p1).arg(p2);
  s += QString(".MODEL MODEL_%1 MLIN(l=%2 w=%3 model=%4 disp=%5 tranmodel=%6 %7%8)\n")
          .arg(Name).arg(L).arg(W).arg(Mod).arg(Disp).arg(Tran).arg(subline).arg(hammerstadParams);

  return s;
}

void MSline::getHammerstadValues(const QString& top, const QString& bottom, double& h1, double& h2, double& t_embed)
{
    static QMap<QPair<QString, QString>, QPair<double, double>> values;
    if (values.isEmpty()) {
        values.insert(qMakePair(QString("M2"), QString("M1")), qMakePair(420.0, 14150.0));
        values.insert(qMakePair(QString("M3"), QString("M1")), qMakePair(1450.0, 14150.0));
        values.insert(qMakePair(QString("M4"), QString("M1")), qMakePair(2480.0, 14150.0));
        values.insert(qMakePair(QString("M5"), QString("M1")), qMakePair(3510.0, 14150.0));
        values.insert(qMakePair(QString("TM1"), QString("M1")), qMakePair(4850.0, 14150.0));
        values.insert(qMakePair(QString("TM2"), QString("M1")), qMakePair(9650.0, 14150.0));
        values.insert(qMakePair(QString("M3"), QString("M2")), qMakePair(540.0, 13240.0));
        values.insert(qMakePair(QString("M4"), QString("M2")), qMakePair(1570.0, 13240.0));
        values.insert(qMakePair(QString("M5"), QString("M2")), qMakePair(2600.0, 13240.0));
        values.insert(qMakePair(QString("TM1"), QString("M2")), qMakePair(3940.0, 13240.0));
        values.insert(qMakePair(QString("TM2"), QString("M2")), qMakePair(8740.0, 13240.0));
        values.insert(qMakePair(QString("M4"), QString("M3")), qMakePair(540.0, 12210.0));
        values.insert(qMakePair(QString("M5"), QString("M3")), qMakePair(1570.0, 12210.0));
        values.insert(qMakePair(QString("TM1"), QString("M3")), qMakePair(2910.0, 12210.0));
        values.insert(qMakePair(QString("TM2"), QString("M3")), qMakePair(7710.0, 12210.0));
        values.insert(qMakePair(QString("M5"), QString("M4")), qMakePair(540.0, 11180.0));
        values.insert(qMakePair(QString("TM1"), QString("M4")), qMakePair(1880.0, 11180.0));
        values.insert(qMakePair(QString("TM2"), QString("M4")), qMakePair(6680.0, 11180.0));
        values.insert(qMakePair(QString("TM1"), QString("M5")), qMakePair(850.0, 10150.0));
        values.insert(qMakePair(QString("TM2"), QString("M5")), qMakePair(5650.0, 10150.0));
        values.insert(qMakePair(QString("TM2"), QString("TM1")), qMakePair(2800.0, 7300.0));
    }

    QPair<double, double> h_vals = values.value(qMakePair(top, bottom));
    h1 = h_vals.first;
    h2 = h_vals.second;

    if (top == "TM2") {
        t_embed = 3000.0;
    } else if (top == "TM1") {
        t_embed = 2000.0;
    } else {
        t_embed = 490.0;
    }
}
