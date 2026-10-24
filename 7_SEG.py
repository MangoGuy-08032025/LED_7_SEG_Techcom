import sys
from PyQt5.QtWidgets import (
    QApplication, QWidget, QCheckBox, QLabel,
    QTextEdit, QRadioButton
)
from PyQt5.QtCore import Qt


class SevenSegGenerator(QWidget):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("7 Segment Code Generator")
        self.setGeometry(100, 100, 700, 400)

        self.mapping = {
            "A": 0,
            "B": 1,
            "C": 2,
            "D": 3,
            "E": 4,
            "F": 5,
            "G": 6,
            "DP": 7
        }

        self.create_ui()
        self.update_code()

    def create_ui(self):

        # Checkbox theo hình dạng LED 7 đoạn

        self.cbA = QCheckBox("A", self)
        self.cbA.move(120, 20)

        self.cbF = QCheckBox("F", self)
        self.cbF.move(40, 80)

        self.cbB = QCheckBox("B", self)
        self.cbB.move(200, 80)

        self.cbG = QCheckBox("G", self)
        self.cbG.move(120, 140)

        self.cbE = QCheckBox("E", self)
        self.cbE.move(40, 200)

        self.cbC = QCheckBox("C", self)
        self.cbC.move(200, 200)

        self.cbD = QCheckBox("D", self)
        self.cbD.move(120, 260)

        self.cbDP = QCheckBox("DP", self)
        self.cbDP.move(260, 260)

        self.checkboxes = {
            "A": self.cbA,
            "B": self.cbB,
            "C": self.cbC,
            "D": self.cbD,
            "E": self.cbE,
            "F": self.cbF,
            "G": self.cbG,
            "DP": self.cbDP
        }

        for cb in self.checkboxes.values():
            cb.stateChanged.connect(self.update_code)

        # Chọn loại LED

        self.rbCathode = QRadioButton("Common Cathode", self)
        self.rbCathode.move(380, 30)
        self.rbCathode.setChecked(True)

        self.rbAnode = QRadioButton("Common Anode", self)
        self.rbAnode.move(380, 60)

        self.rbCathode.toggled.connect(self.update_code)
        self.rbAnode.toggled.connect(self.update_code)

        # Kết quả

        self.lblBin = QLabel("BIN:", self)
        self.lblBin.move(380, 110)

        self.lblHex = QLabel("HEX:", self)
        self.lblHex.move(380, 140)

        self.lblDec = QLabel("DEC:", self)
        self.lblDec.move(380, 170)

        self.codeBox = QTextEdit(self)
        self.codeBox.setGeometry(380, 210, 280, 120)

    def update_code(self):

        value = 0

        for seg, checkbox in self.checkboxes.items():
            if checkbox.isChecked():
                bit = self.mapping[seg]
                value |= (1 << bit)

        # Common Anode
        if self.rbAnode.isChecked():
            value ^= 0xFF

        binary = format(value, '08b')
        hexa = f"0x{value:02X}"
        decimal = str(value)

        self.lblBin.setText(f"BIN : {binary}")
        self.lblHex.setText(f"HEX : {hexa}")
        self.lblDec.setText(f"DEC : {decimal}")

        code = (
            f"// Generated Code\n"
            f"uint8_t segCode = {hexa};\n\n"
            f"shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, {hexa});"
        )

        self.codeBox.setText(code)


if __name__ == "__main__":
    app = QApplication(sys.argv)

    window = SevenSegGenerator()
    window.show()

    sys.exit(app.exec_())