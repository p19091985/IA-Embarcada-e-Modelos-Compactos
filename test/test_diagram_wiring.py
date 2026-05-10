#!/usr/bin/env python3
"""
Teste de validacao do diagram.json contra o codigo main.cpp.
Garante que todos os componentes (2 LCDs, 2 displays de 7 segmentos, CD4511,
sensor, resistores) estao corretamente conectados aos pinos que o firmware espera.

Esse teste impede que alteracoes no diagrama quebrem a simulacao Wokwi.
"""

import json
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
DIAGRAM = ROOT / "diagram.json"
MAIN_CPP = ROOT / "main" / "main.cpp"
WOKWI_TOML = ROOT / "wokwi.toml"
CHIP_WASM = ROOT / "main" / "cd4511.chip.wasm"
CHIP_JSON = ROOT / "main" / "cd4511.chip.json"


def carregar_diagrama():
    with DIAGRAM.open() as f:
        return json.load(f)


def carregar_defines_main():
    """Extrai todos os #define GPIO_NUM_XX do main.cpp"""
    text = MAIN_CPP.read_text(encoding="utf-8")
    defines = {}
    for match in re.finditer(r'#define\s+(\w+)\s+GPIO_NUM_(\d+)', text):
        defines[match.group(1)] = int(match.group(2))
    # Extrair endereco I2C
    m = re.search(r'#define\s+ENDERECO_LCD_PRINCIPAL\s+(0x[0-9a-fA-F]+)', text)
    if m:
        defines["ENDERECO_LCD_PRINCIPAL"] = int(m.group(1), 16)
    return defines


def encontrar_parte(diagrama, part_id):
    for part in diagrama["parts"]:
        if part["id"] == part_id:
            return part
    return None


def encontrar_conexao(diagrama, pin_a, pin_b=None):
    """Procura uma conexao que contenha pin_a (e opcionalmente pin_b)."""
    for conn in diagrama["connections"]:
        if pin_a in conn[0] or pin_a in conn[1]:
            if pin_b is None:
                return conn
            if pin_b in conn[0] or pin_b in conn[1]:
                return conn
    return None


def conexoes_do_componente(diagrama, comp_id):
    """Retorna todas as conexoes que envolvem um componente."""
    prefix = comp_id + ":"
    result = []
    for conn in diagrama["connections"]:
        if conn[0].startswith(prefix) or conn[1].startswith(prefix):
            result.append(conn)
    return result


class TestDiagramaComponentes(unittest.TestCase):
    """Verifica que todos os componentes do BOM existem no diagrama."""

    @classmethod
    def setUpClass(cls):
        cls.diag = carregar_diagrama()
        cls.parts = {p["id"]: p for p in cls.diag["parts"]}

    def test_esp32_presente(self):
        esp = self.parts.get("esp")
        self.assertIsNotNone(esp, "ESP32-S3 ausente do diagrama")
        self.assertEqual(esp["type"], "board-esp32-s3-devkitc-1")

    def test_lcd_i2c_presente_e_configurado(self):
        lcd = self.parts.get("lcd_i2c")
        self.assertIsNotNone(lcd, "LCD I2C ausente do diagrama")
        self.assertEqual(lcd["type"], "wokwi-lcd1602")
        self.assertEqual(lcd["attrs"]["pins"], "i2c")
        self.assertEqual(lcd["attrs"]["i2cAddress"], "0x27",
                         "Endereco I2C do LCD deve ser 0x27 (padrao Wokwi para PCF8574)")

    def test_lcd_parallel_presente(self):
        lcd = self.parts.get("lcd_parallel")
        self.assertIsNotNone(lcd, "LCD paralelo ausente do diagrama")
        self.assertEqual(lcd["type"], "wokwi-lcd1602")

    def test_digit1_presente_e_catodico(self):
        """digit1 e o display via CD4511 (decodificador BCD)"""
        d = self.parts.get("digit1")
        self.assertIsNotNone(d, "Display 7-segmentos digit1 ausente")
        self.assertEqual(d["type"], "wokwi-7segment")
        self.assertEqual(d["attrs"]["common"], "cathode")
        self.assertEqual(d["attrs"]["color"], "red")

    def test_digit2_presente_e_catodico(self):
        """digit2 e o display acionado diretamente pelo ESP32"""
        d = self.parts.get("digit2")
        self.assertIsNotNone(d, "Display 7-segmentos digit2 ausente")
        self.assertEqual(d["type"], "wokwi-7segment")
        self.assertEqual(d["attrs"]["common"], "cathode")
        self.assertEqual(d["attrs"]["color"], "red")

    def test_cd4511_presente(self):
        u = self.parts.get("u4511")
        self.assertIsNotNone(u, "Chip CD4511 ausente do diagrama")
        self.assertEqual(u["type"], "chip-cd4511")

    def test_sensor_presente(self):
        s = self.parts.get("soil1")
        self.assertIsNotNone(s, "Sensor de umidade ausente do diagrama")
        self.assertEqual(s["type"], "wokwi-slide-potentiometer")

    def test_leds_de_irrigacao_presentes(self):
        led_linear = self.parts.get("led_irrigacao_linear")
        self.assertIsNotNone(led_linear, "LED de irrigação linear ausente")
        self.assertEqual(led_linear["type"], "wokwi-led")
        self.assertEqual(led_linear["attrs"]["color"], "blue")

        led_ia = self.parts.get("led_irrigacao_ia")
        self.assertIsNotNone(led_ia, "LED de irrigação IA ausente")
        self.assertEqual(led_ia["type"], "wokwi-led")
        self.assertEqual(led_ia["attrs"]["color"], "yellow")
        self.assertEqual(led_ia["attrs"]["lightColor"], "#ffd700")

    def test_resistores_presentes(self):
        """BOM: R1(100), R2(220), R3-R12(330, exceto R1/R2)"""
        self.assertIn("r1", self.parts)
        self.assertEqual(self.parts["r1"]["attrs"]["value"], "100")

        self.assertIn("r2", self.parts)
        self.assertEqual(self.parts["r2"]["attrs"]["value"], "220")

        for rid in ["r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12"]:
            self.assertIn(rid, self.parts, f"Resistor {rid} ausente")
            self.assertEqual(self.parts[rid]["attrs"]["value"], "330",
                             f"Resistor {rid} deve ser 330 ohms")

    def test_rotulos_de_medicao_presentes(self):
        self.assertEqual(self.parts["label_lcd_ia"]["type"], "wokwi-text")
        self.assertEqual(self.parts["label_lcd_ia"]["attrs"]["text"], "Medição por IA")
        self.assertEqual(self.parts["label_lcd_linear"]["type"], "wokwi-text")
        self.assertEqual(self.parts["label_lcd_linear"]["attrs"]["text"], "Medição linear")
        self.assertEqual(self.parts["label_digit_ia"]["attrs"]["text"], "7 segmentos IA")
        self.assertEqual(self.parts["label_digit_linear"]["attrs"]["text"], "7 segmentos linear")
        self.assertEqual(self.parts["label_led_linear"]["attrs"]["text"], "LED azul\nIrrigação linear")
        self.assertEqual(self.parts["label_led_ia"]["attrs"]["text"], "LED dourado\nIrrigação por IA")

    def test_total_de_partes(self):
        """BOM + LEDs de irrigação + rótulos Wokwi."""
        self.assertEqual(len(self.diag["parts"]), 27,
                         "Numero de partes no diagrama nao bate com o BOM do legado")


class TestDiagramaConexoesLcdI2C(unittest.TestCase):
    """Verifica que o LCD I2C esta conectado nos pinos corretos (SDA=1, SCL=2)."""

    @classmethod
    def setUpClass(cls):
        cls.diag = carregar_diagrama()
        cls.defines = carregar_defines_main()

    def test_sda_conectado_ao_gpio1(self):
        sda_gpio = self.defines["PINO_SDA_LCD_PRINCIPAL"]
        conn = encontrar_conexao(self.diag, "lcd_i2c:SDA", f"esp:{sda_gpio}")
        self.assertIsNotNone(conn, f"LCD I2C SDA deve estar conectado em esp:{sda_gpio}")

    def test_scl_conectado_ao_gpio2(self):
        scl_gpio = self.defines["PINO_SCL_LCD_PRINCIPAL"]
        conn = encontrar_conexao(self.diag, "lcd_i2c:SCL", f"esp:{scl_gpio}")
        self.assertIsNotNone(conn, f"LCD I2C SCL deve estar conectado em esp:{scl_gpio}")

    def test_vcc_e_gnd_conectados(self):
        self.assertIsNotNone(encontrar_conexao(self.diag, "lcd_i2c:VCC"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "lcd_i2c:GND"))

    def test_endereco_i2c_bate_com_codigo(self):
        diag = carregar_diagrama()
        lcd = encontrar_parte(diag, "lcd_i2c")
        endereco_diag = int(lcd["attrs"]["i2cAddress"], 16)
        endereco_code = self.defines["ENDERECO_LCD_PRINCIPAL"]
        self.assertEqual(endereco_diag, endereco_code,
                         f"Endereco I2C no diagrama (0x{endereco_diag:02x}) "
                         f"diferente do codigo (0x{endereco_code:02x})")


class TestDiagramaConexoesLcdParalelo(unittest.TestCase):
    """Verifica que o LCD paralelo esta conectado nos pinos corretos."""

    @classmethod
    def setUpClass(cls):
        cls.diag = carregar_diagrama()
        cls.defines = carregar_defines_main()

    def test_rs_conectado(self):
        gpio = self.defines["PINO_LCD_PAR_RS"]
        conn = encontrar_conexao(self.diag, "lcd_parallel:RS", f"esp:{gpio}")
        self.assertIsNotNone(conn, f"LCD paralelo RS deve estar em esp:{gpio}")

    def test_e_conectado(self):
        gpio = self.defines["PINO_LCD_PAR_E"]
        conn = encontrar_conexao(self.diag, "lcd_parallel:E", f"esp:{gpio}")
        self.assertIsNotNone(conn, f"LCD paralelo E deve estar em esp:{gpio}")

    def test_d4_conectado(self):
        gpio = self.defines["PINO_LCD_PAR_D4"]
        conn = encontrar_conexao(self.diag, "lcd_parallel:D4", f"esp:{gpio}")
        self.assertIsNotNone(conn, f"LCD paralelo D4 deve estar em esp:{gpio}")

    def test_d5_conectado(self):
        gpio = self.defines["PINO_LCD_PAR_D5"]
        conn = encontrar_conexao(self.diag, "lcd_parallel:D5", f"esp:{gpio}")
        self.assertIsNotNone(conn, f"LCD paralelo D5 deve estar em esp:{gpio}")

    def test_d6_conectado(self):
        gpio = self.defines["PINO_LCD_PAR_D6"]
        conn = encontrar_conexao(self.diag, "lcd_parallel:D6", f"esp:{gpio}")
        self.assertIsNotNone(conn, f"LCD paralelo D6 deve estar em esp:{gpio}")

    def test_d7_conectado(self):
        gpio = self.defines["PINO_LCD_PAR_D7"]
        conn = encontrar_conexao(self.diag, "lcd_parallel:D7", f"esp:{gpio}")
        self.assertIsNotNone(conn, f"LCD paralelo D7 deve estar em esp:{gpio}")

    def test_alimentacao_lcd_paralelo(self):
        self.assertIsNotNone(encontrar_conexao(self.diag, "lcd_parallel:VDD"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "lcd_parallel:VSS"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "lcd_parallel:A"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "lcd_parallel:K"))


class TestDiagramaConexoesBCD(unittest.TestCase):
    """Verifica que o CD4511 (digit1) esta conectado nos pinos BCD corretos."""

    @classmethod
    def setUpClass(cls):
        cls.diag = carregar_diagrama()
        cls.defines = carregar_defines_main()

    def test_bcd_a_conectado(self):
        gpio = self.defines["PINO_BCD_A"]
        conn = encontrar_conexao(self.diag, f"esp:{gpio}", "u4511:A")
        self.assertIsNotNone(conn, f"BCD A deve estar em esp:{gpio} -> u4511:A")

    def test_bcd_b_conectado(self):
        gpio = self.defines["PINO_BCD_B"]
        conn = encontrar_conexao(self.diag, f"esp:{gpio}", "u4511:B")
        self.assertIsNotNone(conn, f"BCD B deve estar em esp:{gpio} -> u4511:B")

    def test_bcd_c_conectado(self):
        gpio = self.defines["PINO_BCD_C"]
        conn = encontrar_conexao(self.diag, f"esp:{gpio}", "u4511:C")
        self.assertIsNotNone(conn, f"BCD C deve estar em esp:{gpio} -> u4511:C")

    def test_bcd_d_conectado(self):
        gpio = self.defines["PINO_BCD_D"]
        conn = encontrar_conexao(self.diag, f"esp:{gpio}", "u4511:D")
        self.assertIsNotNone(conn, f"BCD D deve estar em esp:{gpio} -> u4511:D")

    def test_cd4511_saidas_para_digit1(self):
        """Todas as 7 saidas do CD4511 devem ir para digit1 (A-G)"""
        for seg in "ABCDEFG":
            conn = encontrar_conexao(self.diag, f"u4511:SEG_{seg}", f"digit1:{seg}")
            self.assertIsNotNone(conn, f"u4511:SEG_{seg} deve conectar a digit1:{seg}")

    def test_cd4511_alimentacao(self):
        self.assertIsNotNone(encontrar_conexao(self.diag, "u4511:VDD"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "u4511:VSS"))

    def test_cd4511_controles(self):
        """LT e BL devem estar em VCC, LE em GND para operacao normal"""
        self.assertIsNotNone(encontrar_conexao(self.diag, "u4511:LT"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "u4511:BL"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "u4511:LE"))

    def test_digit1_catodo_com_r1(self):
        """digit1:COM -> r1 -> GND (resistor de 100 ohms)"""
        self.assertIsNotNone(encontrar_conexao(self.diag, "digit1:COM", "r1:1"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "r1:2"))


class TestDiagramaConexoes7SegDireto(unittest.TestCase):
    """Verifica que o digit2 (acionado diretamente) esta nos pinos corretos."""

    @classmethod
    def setUpClass(cls):
        cls.diag = carregar_diagrama()
        cls.defines = carregar_defines_main()

    def _verifica_segmento(self, seg_name, define_name, resistor_id, digit_pin):
        """Verifica: esp:GPIO -> resistor -> digit2:SEGMENTO"""
        gpio = self.defines[define_name]
        conn1 = encontrar_conexao(self.diag, f"esp:{gpio}", f"{resistor_id}:1")
        self.assertIsNotNone(conn1,
            f"{seg_name}: esp:{gpio} deve conectar a {resistor_id}:1")
        conn2 = encontrar_conexao(self.diag, f"{resistor_id}:2", f"digit2:{digit_pin}")
        self.assertIsNotNone(conn2,
            f"{seg_name}: {resistor_id}:2 deve conectar a digit2:{digit_pin}")

    def test_segmento_a(self):
        self._verifica_segmento("SEG_A", "PINO_SEG_A", "r10", "A")

    def test_segmento_b(self):
        self._verifica_segmento("SEG_B", "PINO_SEG_B", "r3", "B")

    def test_segmento_c(self):
        self._verifica_segmento("SEG_C", "PINO_SEG_C", "r9", "C")

    def test_segmento_d(self):
        self._verifica_segmento("SEG_D", "PINO_SEG_D", "r8", "D")

    def test_segmento_e(self):
        self._verifica_segmento("SEG_E", "PINO_SEG_E", "r6", "E")

    def test_segmento_f(self):
        self._verifica_segmento("SEG_F", "PINO_SEG_F", "r4", "F")

    def test_segmento_g(self):
        self._verifica_segmento("SEG_G", "PINO_SEG_G", "r5", "G")

    def test_ponto_decimal(self):
        gpio = self.defines["PINO_SEG_DP"]
        conn1 = encontrar_conexao(self.diag, f"esp:{gpio}", "r7:1")
        self.assertIsNotNone(conn1, f"DP: esp:{gpio} deve conectar a r7:1")
        conn2 = encontrar_conexao(self.diag, "r7:2", "digit2:DP")
        self.assertIsNotNone(conn2, "DP: r7:2 deve conectar a digit2:DP")

    def test_digit2_catodo_ao_gnd(self):
        conn = encontrar_conexao(self.diag, "digit2:COM")
        self.assertIsNotNone(conn, "digit2:COM deve estar conectado ao GND")


class TestDiagramaConexoesSensor(unittest.TestCase):
    """Verifica que o sensor de umidade esta no pino correto."""

    @classmethod
    def setUpClass(cls):
        cls.diag = carregar_diagrama()
        cls.defines = carregar_defines_main()

    def test_sensor_sinal_no_gpio4(self):
        gpio = self.defines["PINO_SENSOR_UMIDADE"]
        conn = encontrar_conexao(self.diag, "soil1:SIG", f"esp:{gpio}")
        self.assertIsNotNone(conn, f"Sensor SIG deve estar em esp:{gpio}")

    def test_sensor_alimentacao(self):
        self.assertIsNotNone(encontrar_conexao(self.diag, "soil1:GND"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "soil1:VCC"))

    def test_resistor_r2_no_sensor(self):
        """R2 (220 ohms) deve estar no circuito do sensor"""
        self.assertIsNotNone(encontrar_conexao(self.diag, "r2:2", "soil1:VCC"))


class TestDiagramaConexoesLedsIrrigacao(unittest.TestCase):
    """Verifica os LEDs indicadores de irrigação por tipo de medição."""

    @classmethod
    def setUpClass(cls):
        cls.diag = carregar_diagrama()
        cls.defines = carregar_defines_main()

    def test_led_linear_no_gpio16(self):
        gpio = self.defines["PINO_LED_IRRIGACAO_LINEAR"]
        self.assertIsNotNone(encontrar_conexao(self.diag, f"esp:{gpio}", "r11:1"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "r11:2", "led_irrigacao_linear:A"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "led_irrigacao_linear:C"))

    def test_led_ia_no_gpio17(self):
        gpio = self.defines["PINO_LED_IRRIGACAO_IA"]
        self.assertIsNotNone(encontrar_conexao(self.diag, f"esp:{gpio}", "r12:1"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "r12:2", "led_irrigacao_ia:A"))
        self.assertIsNotNone(encontrar_conexao(self.diag, "led_irrigacao_ia:C"))


class TestArquivosWokwi(unittest.TestCase):
    """Verifica que os arquivos da simulacao Wokwi estao presentes."""

    def test_wokwi_toml_existe(self):
        self.assertTrue(WOKWI_TOML.exists())

    def test_chip_wasm_existe(self):
        self.assertTrue(CHIP_WASM.exists(), "cd4511.chip.wasm ausente na pasta main")

    def test_chip_json_existe(self):
        self.assertTrue(CHIP_JSON.exists(), "cd4511.chip.json ausente na pasta main")

    def test_wokwi_toml_aponta_para_main(self):
        text = WOKWI_TOML.read_text(encoding="utf-8")
        self.assertIn("main/cd4511.chip.wasm", text,
                      "wokwi.toml deve apontar para main/cd4511.chip.wasm")

    def test_diagram_json_valido(self):
        """diagram.json deve ser JSON valido"""
        with DIAGRAM.open() as f:
            data = json.load(f)
        self.assertIn("parts", data)
        self.assertIn("connections", data)
        self.assertGreater(len(data["parts"]), 0)
        self.assertGreater(len(data["connections"]), 0)


class TestSerialMonitor(unittest.TestCase):
    """Verifica configuracao do serial monitor."""

    def test_serial_monitor_configurado(self):
        diag = carregar_diagrama()
        self.assertIn("serialMonitor", diag)

    def test_serial_tx_rx_conectados(self):
        diag = carregar_diagrama()
        tx = encontrar_conexao(diag, "esp:TX", "$serialMonitor:RX")
        rx = encontrar_conexao(diag, "esp:RX", "$serialMonitor:TX")
        self.assertIsNotNone(tx, "ESP TX deve conectar ao serial monitor RX")
        self.assertIsNotNone(rx, "ESP RX deve conectar ao serial monitor TX")


if __name__ == "__main__":
    unittest.main()
