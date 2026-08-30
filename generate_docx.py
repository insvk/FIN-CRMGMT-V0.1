#!/usr/bin/env python3
"""
Generates CRMGMT_v0.1_Login_Credentials.docx using pure standard Python zipfile & OpenXML.
"""

import zipfile
import os

OUTPUT_DOCX = os.path.join(os.path.dirname(__file__), "CRMGMT_v0.1_Login_Credentials.docx")

def build_docx(filename):
    content_types_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
    <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
    <Default Extension="xml" ContentType="application/xml"/>
    <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
    <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>"""

    rels_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
    <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>"""

    document_rels_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
    <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>"""

    styles_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
    <w:docDefaults>
        <w:rPrDefault>
            <w:rPr>
                <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri" w:cs="Calibri"/>
                <w:sz w:val="22"/>
                <w:szCs w:val="22"/>
                <w:color w:val="2B354F"/>
            </w:rPr>
        </w:rPrDefault>
    </w:docDefaults>
</w:styles>"""

    document_xml = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
    <w:body>
        <!-- Header Title -->
        <w:p>
            <w:pPr>
                <w:spacing w:before="100" w:after="100"/>
                <w:jc w:val="center"/>
            </w:pPr>
            <w:r>
                <w:rPr>
                    <w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI"/>
                    <w:b/>
                    <w:sz w:val="48"/>
                    <w:color w:val="0D3B66"/>
                </w:rPr>
                <w:t>SAVEETHA INSTITUTE OF MEDICAL AND TECHNICAL SCIENCES</w:t>
            </w:r>
        </w:p>

        <!-- Subtitle -->
        <w:p>
            <w:pPr>
                <w:spacing w:before="0" w:after="240"/>
                <w:jc w:val="center"/>
            </w:pPr>
            <w:r>
                <w:rPr>
                    <w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI"/>
                    <w:b/>
                    <w:sz w:val="28"/>
                    <w:color w:val="3B7DDD"/>
                </w:rPr>
                <w:t>CRMGMT v0.1 - Master System Login Credentials &amp; Access Guide</w:t>
            </w:r>
        </w:p>

        <!-- Section 1: Overview Callout -->
        <w:p>
            <w:pPr>
                <w:spacing w:before="120" w:after="120"/>
            </w:pPr>
            <w:r>
                <w:rPr><w:b/><w:sz w:val="24"/><w:color w:val="1E293B"/></w:rPr>
                <w:t>1. Access Endpoints &amp; Portal Information</w:t>
            </w:r>
        </w:p>

        <w:p>
            <w:r><w:rPr><w:b/></w:rPr><w:t>• Web Application URL: </w:t></w:r>
            <w:r><w:rPr><w:color w:val="3B7DDD"/></w:rPr><w:t>http://127.0.0.1:8080</w:t></w:r>
        </w:p>
        <w:p>
            <w:r><w:rPr><w:b/></w:rPr><w:t>• REST API Base: </w:t></w:r>
            <w:r><w:rPr><w:color w:val="3B7DDD"/></w:rPr><w:t>http://127.0.0.1:8080/api/v1</w:t></w:r>
        </w:p>
        <w:p>
            <w:r><w:rPr><w:b/></w:rPr><w:t>• Default Auth Method: </w:t></w:r>
            <w:r><w:t>Bearer JWT (HMAC-SHA256) / Password (PBKDF2-SHA256)</w:t></w:r>
        </w:p>

        <!-- Section 2: Master Credentials Table -->
        <w:p>
            <w:pPr>
                <w:spacing w:before="240" w:after="120"/>
            </w:pPr>
            <w:r>
                <w:rPr><w:b/><w:sz w:val="24"/><w:color w:val="1E293B"/></w:rPr>
                <w:t>2. User Accounts &amp; Passwords Directory</w:t>
            </w:r>
        </w:p>

        <w:tbl>
            <w:tblPr>
                <w:tblW w:w="9600" w:type="dxa"/>
                <w:tblBorders>
                    <w:top w:val="single" w:sz="6" w:space="0" w:color="CBD5E1"/>
                    <w:left w:val="single" w:sz="6" w:space="0" w:color="CBD5E1"/>
                    <w:bottom w:val="single" w:sz="6" w:space="0" w:color="CBD5E1"/>
                    <w:right w:val="single" w:sz="6" w:space="0" w:color="CBD5E1"/>
                    <w:insideH w:val="single" w:sz="4" w:space="0" w:color="E2E8F0"/>
                    <w:insideV w:val="single" w:sz="4" w:space="0" w:color="E2E8F0"/>
                </w:tblBorders>
            </w:tblPr>

            <!-- Table Header -->
            <w:tr>
                <w:trPr><w:tblHeader/></w:trPr>
                <w:tc>
                    <w:tcPr><w:shd w:val="clear" w:color="auto" w:fill="1C2333"/><w:tcW w:w="1600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>Role</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:shd w:val="clear" w:color="auto" w:fill="1C2333"/><w:tcW w:w="1800" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>Full Name</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:shd w:val="clear" w:color="auto" w:fill="1C2333"/><w:tcW w:w="2600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>Email / Username</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:shd w:val="clear" w:color="auto" w:fill="1C2333"/><w:tcW w:w="1600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>Password</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:shd w:val="clear" w:color="auto" w:fill="1C2333"/><w:tcW w:w="2000" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>Master / API Key</w:t></w:r></w:p>
                </w:tc>
            </w:tr>

            <!-- Row 1: Super Admin -->
            <w:tr>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="3B7DDD"/></w:rPr><w:t>Super Admin</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1800" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>SIMATS Chief Systems Administrator</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>admin@crmgmt.io</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="E0F9F1"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="065F46"/></w:rPr><w:t>Admin@123</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2000" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>crm_master_key_8f3a9e22</w:t></w:r></w:p>
                </w:tc>
            </w:tr>

            <!-- Row 2: Hub Manager -->
            <w:tr>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="0D3B66"/></w:rPr><w:t>Hub Manager</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1800" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>Saveetha Central Hub Operations Manager</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>hub.chennai@crmgmt.io</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="E0F9F1"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="065F46"/></w:rPr><w:t>Admin@123</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2000" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>crm_hub_che_key_44b1c</w:t></w:r></w:p>
                </w:tc>
            </w:tr>

            <!-- Row 3: Delivery Agent -->
            <w:tr>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="7C3AED"/></w:rPr><w:t>Delivery Agent</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1800" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>SIMATS Dispatch &amp; Fleet Unit 01</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>agent.che01@crmgmt.io</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="E0F9F1"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="065F46"/></w:rPr><w:t>Admin@123</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2000" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>crm_agent_che_12345</w:t></w:r></w:p>
                </w:tc>
            </w:tr>

            <!-- Row 4: Enterprise Customer -->
            <w:tr>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="059669"/></w:rPr><w:t>Enterprise</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1800" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>Saveetha Biomedical &amp; Healthcare Procurement</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>enterprise@saveetha.com</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="E0F9F1"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="065F46"/></w:rPr><w:t>Admin@123</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2000" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>crm_ent_sav_99a8b7</w:t></w:r></w:p>
                </w:tc>
            </w:tr>

            <!-- Row 5: Standard Customer -->
            <w:tr>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="D97706"/></w:rPr><w:t>Customer</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1800" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>Verified Retail Consignee</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>customer@gmail.com</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="1600" w:type="dxa"/><w:shd w:val="clear" w:color="auto" w:fill="FEF3C7"/></w:tcPr>
                    <w:p><w:r><w:rPr><w:b/><w:color w:val="92400E"/></w:rPr><w:t>User@123</w:t></w:r></w:p>
                </w:tc>
                <w:tc>
                    <w:tcPr><w:tcW w:w="2000" w:type="dxa"/></w:tcPr>
                    <w:p><w:r><w:t>N/A</w:t></w:r></w:p>
                </w:tc>
            </w:tr>
        </w:tbl>

        <!-- Section 3: Live Demo Tracking Consignments -->
        <w:p>
            <w:pPr>
                <w:spacing w:before="240" w:after="120"/>
            </w:pPr>
            <w:r>
                <w:rPr><w:b/><w:sz w:val="24"/><w:color w:val="1E293B"/></w:rPr>
                <w:t>3. Preloaded Consignments &amp; Checksum Tracking IDs</w:t>
            </w:r>
        </w:p>

        <w:p>
            <w:r><w:rPr><w:b/><w:color w:val="3B7DDD"/></w:rPr><w:t>1. CR-68D3F12A-B4-9F81</w:t></w:r>
            <w:r><w:t> - Saveetha Biomed Lab to Apollo Hospital (Status: </w:t></w:r>
            <w:r><w:rPr><w:b/><w:color w:val="059669"/></w:rPr><w:t>OUT FOR DELIVERY</w:t></w:r>
            <w:r><w:t> | Route: Chennai Urban 4B)</w:t></w:r>
        </w:p>

        <w:p>
            <w:r><w:rPr><w:b/><w:color w:val="3B7DDD"/></w:rPr><w:t>2. CR-68D40E1B-C2-4E29</w:t></w:r>
            <w:r><w:t> - Ananya Sharma to Hyderabad Cyber Gateway (Status: </w:t></w:r>
            <w:r><w:rPr><w:b/><w:color w:val="2563EB"/></w:rPr><w:t>IN TRANSIT</w:t></w:r>
            <w:r><w:t> | Route: NH44 Corridor)</w:t></w:r>
        </w:p>

        <w:p>
            <w:r><w:rPr><w:b/><w:color w:val="3B7DDD"/></w:rPr><w:t>3. CR-68D41A9C-7F-33A1</w:t></w:r>
            <w:r><w:t> - Saveetha Tech R&amp;D to Infosys Bangalore (Status: </w:t></w:r>
            <w:r><w:rPr><w:b/><w:color w:val="D97706"/></w:rPr><w:t>PICKED UP</w:t></w:r>
            <w:r><w:t> | Route: Chennai Central)</w:t></w:r>
        </w:p>

        <w:p>
            <w:r><w:rPr><w:b/><w:color w:val="3B7DDD"/></w:rPr><w:t>4. CR-68D4255E-A1-19B4</w:t></w:r>
            <w:r><w:t> - TechNova Mumbai to Meera Krishnan Chennai (Status: </w:t></w:r>
            <w:r><w:rPr><w:b/><w:color w:val="10B981"/></w:rPr><w:t>DELIVERED</w:t></w:r>
            <w:r><w:t> | POD Verified)</w:t></w:r>
        </w:p>

        <!-- Section 4: Testing Features -->
        <w:p>
            <w:pPr>
                <w:spacing w:before="240" w:after="120"/>
            </w:pPr>
            <w:r>
                <w:rPr><w:b/><w:sz w:val="24"/><w:color w:val="1E293B"/></w:rPr>
                <w:t>4. One-Click Role Evaluation Tips</w:t>
            </w:r>
        </w:p>

        <w:p>
            <w:r><w:rPr><w:b/></w:rPr><w:t>• 1-Click Login: </w:t></w:r>
            <w:r><w:t>On the login page, click any button in the "⚡ Quick-Fill Demo Roles" bar to immediately populate the corresponding account credentials.</w:t></w:r>
        </w:p>
        <w:p>
            <w:r><w:rPr><w:b/></w:rPr><w:t>• Floating Role Switcher: </w:t></w:r>
            <w:r><w:t>While logged in, use the floating dropdown widget at the bottom-left to dynamically test Admin, Hub Manager, Agent, and Customer views without re-authenticating.</w:t></w:r>
        </w:p>
    </w:body>
</w:document>"""

    with zipfile.ZipFile(filename, 'w', zipfile.ZIP_DEFLATED) as docx:
        docx.writestr('[Content_Types].xml', content_types_xml)
        docx.writestr('_rels/.rels', rels_xml)
        docx.writestr('word/_rels/document.xml.rels', document_rels_xml)
        docx.writestr('word/document.xml', document_xml)
        docx.writestr('word/styles.xml', styles_xml)

    print(f"Successfully generated: {filename}")

if __name__ == '__main__':
    build_docx(OUTPUT_DOCX)
