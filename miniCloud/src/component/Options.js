import React from 'react';
import MenuBar from './MenuBar';

import { Card, Col, Row } from 'antd';
const { Meta } = Card;


const Options = (props) => {
  const { click_mailbox, click_storage, click_admin } = props;
  return(
    <>
    
      <div className="site-card-wrapper">
      <Row gutter={16}>
        <Col span={8}>
          <Card
            hoverable
            style={{
              width: 240,
              margin: "100px 100px auto auto"
            }}
            onClick={()=>click_mailbox()}
            cover={<img alt="example" src={require("../img/iphone_apps.jpg")} />}
          >
            <Meta title="Email" description="Email System" />
          </Card>
        </Col>
        <Col span={8}>
          <Card
            hoverable
            style={{
              width: 240,
              margin: "100px auto auto auto"
            }}
            onClick={()=>click_storage()}
            cover={<img alt="example" src={require("../img/cloud-storage-for-wendy2.png")} />}
          >
            <Meta title="Storage" description="Cloud Storage" />
          </Card>
        </Col>
        <Col span={8}>
          <Card
            hoverable
            style={{
              width: 240,
              margin: "100px auto auto auto"
            }}
            onClick={()=>click_admin()}
            cover={<img alt="example" src={require("../img/server-admin.jpg")} />}
          >
            <Meta title="Admin" description="Server Administration" />
          </Card>
        </Col>
      </Row>
    </div>
    </>
  )
};
export default Options;